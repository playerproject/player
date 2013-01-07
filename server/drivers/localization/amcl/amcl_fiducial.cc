/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: AMCL fiducial routines
// Author: David Feil-Seifer
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include "config.h"

#define PLAYER_ENABLE_MSG 1

#include <sys/types.h> // required by Darwin
#include <netinet/in.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <libplayercore/playercore.h>
#include "amcl_fiducial.h"

extern int global_playerport; // used to gen. useful output & debug


#if 0
////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLFiducial::AMCLFiducial(player_device_id_t id)
{
  this->driver = NULL;
  this->fiducial_id = id;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load laser settings
int AMCLFiducial::Load(ConfigFile* cf, int section)
{
  char *map_filename;

  // Device stuff
  //this->fiducial_index = cf->ReadInt(section, "fiducial_index", -1);

  // Get the map settings.  Don't error check here; we'll do it later, in
  // SetupMap().
  cf->ReadDeviceId(&(this->map_id), section, "requires",
                   PLAYER_MAP_CODE, -1, "fiducial");

  // Get the map settings
  map_filename = (char*) cf->ReadFilename(section, "fiducial_map", NULL);
  if( read_map_file( map_filename ) < 0 )
  {
    PLAYER_ERROR( "read_map_file failed" );
  	return -1;
  }

  this->laser_pose.v[0] = cf->ReadTupleLength(section, "laser_pose", 0, 0);
  this->laser_pose.v[1] = cf->ReadTupleLength(section, "laser_pose", 1, 0);
  this->laser_pose.v[2] = cf->ReadTupleAngle(section, "laser_pose", 2, 0);

  this->range_max = cf->ReadLength(section, "laser_range_max", 8.192);
  this->range_var = cf->ReadLength(section, "laser_range_var", 0.1);
  this->angle_var = (M_PI/360.0) * cf->ReadInt(section, "laser_angle_var", 1 );
  this->range_bad = cf->ReadFloat(section, "laser_range_bad", 0.10);
  this->angle_bad = cf->ReadFloat(section, "laser_angle_bad", 0.03);


  this->time.tv_sec = 0;
  this->time.tv_usec = 0;

  return 0;
}

//load the fmap
int AMCLFiducial::read_map_file(const char* map_filename)
{

	FILE * map_file = fopen( map_filename, "r" );

    if( !(map_file) )
    {
        PLAYER_ERROR1( "map file: %s cannot be found\n", map_filename );
        return -1;
    }

    //parse fiducial config file
	int num_landmarks;
    fscanf( map_file, "%d\n", &(num_landmarks) );

    int x = num_landmarks;
	this->fmap = fiducial_map_alloc();
	this->fmap->fiducial_count = x;

    for( int i = 0; i < x; i++ )
    {
		int x, y, id;

        fscanf( map_file, "%d", &id);
		this->fmap->fiducials[i][2] = (double) id;
        fscanf( map_file, "%d", &(x ));
		this->fmap->fiducials[i][0] = (double) x / 1000.0;
        fscanf( map_file, "%d\n", &(y) );
		this->fmap->fiducials[i][1] = (double) y / 1000.0;
    }
	return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLFiducial::Unload(void)
{
  //laser_free(this->model);
  //this->model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int AMCLFiducial::Setup(void)
{

  if(this->SetupMap() < 0)
  {
    PLAYER_ERROR("failed to get laser map");
    return(-1);
  }

  // Subscribe to the Laser device
  this->driver = deviceTable->GetDriver(this->fiducial_id);
  if (!this->driver)
  {
    PLAYER_ERROR("unable to locate suitable fiducial device");
    return -1;
  }
  if (this->driver->Subscribe(this->fiducial_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to fiducial device");
    return -1;
  }

  return 0;
}

// TODO: should Unsubscribe from the map on error returns in the function
int
AMCLFiducial::SetupMap(void)
{
  Driver* mapdriver;

  // Subscribe to the map device
  if(!(mapdriver = deviceTable->GetDriver(this->map_id)))
  {
    PLAYER_ERROR("unable to locate suitable map device");
    return -1;
  }
  if(mapdriver->Subscribe(this->map_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to map device");
    return -1;
  }

  // Create the map
  this->map = map_alloc();
  //PLAYER_MSG1("loading map file [%s]", map_filename);
  PLAYER_MSG1(2, "reading map from map:%d", this->map_id.index);
  //if(map_load_occ(this->map, map_filename, map_scale, map_negate) != 0)
    //return -1;

  // Fill in the map structure (I'm doing it here instead of in libmap,
  // because libmap is written in C, so it'd be a pain to invoke the internal
  // device API from there)

  // first, get the map info
  int replen;
  unsigned short reptype;
  player_map_info_t info;
  struct timeval ts;
  info.subtype = PLAYER_MAP_REQ_GET_INFO;
  if((replen = mapdriver->Request(this->map_id, this,
                                  &info, sizeof(info.subtype), NULL,
                                  &reptype, &info, sizeof(info), &ts)) == 0)
  {
    PLAYER_ERROR("failed to get map info");
    return(-1);
  }

  // copy in the map info
  this->map->origin_x = this->map->origin_y = 0.0;
  this->map->scale = 1/(ntohl(info.scale) / 1e3);
  this->map->size_x = ntohl(info.width);
  this->map->size_y = ntohl(info.height);

  this->map->data_range = 1;

  // allocate space for map cells
  assert(this->map->cells = (map_cell_t*)malloc(sizeof(map_cell_t) *
                                                this->map->size_x *
                                                this->map->size_y));

  // now, get the map data
  player_map_data_t data_req;
  int reqlen;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;

  data_req.subtype = PLAYER_MAP_REQ_GET_DATA;

  // Tile size
  sy = sx = (int)sqrt(sizeof(data_req.data));
  assert(sx * sy < (int)sizeof(data_req.data));
  oi=oj=0;
  while((oi < this->map->size_x) && (oj < this->map->size_y))
  {
    si = MIN(sx, this->map->size_x - oi);
    sj = MIN(sy, this->map->size_y - oj);

    data_req.col = htonl(oi);
    data_req.row = htonl(oj);
    data_req.width = htonl(si);
    data_req.height = htonl(sj);

    reqlen = sizeof(data_req) - sizeof(data_req.data);

    if((replen = mapdriver->Request(this->map_id, this,
                                    &data_req, reqlen, NULL,
                                    &reptype,
                                    &data_req, sizeof(data_req), &ts)) == 0)
    {
      PLAYER_ERROR("failed to get map info");
      return(-1);
    }
    else if(replen != (reqlen + si * sj))
    {
      PLAYER_ERROR2("got less map data than expected (%d != %d)",
                    replen, reqlen + si*sj);
      return(-1);
    }

    // copy the map data
    for(j=0;j<sj;j++)
    {
      for(i=0;i<si;i++)
      {
        this->map->cells[MAP_INDEX(this->map,oi+i,oj+j)].occ_state =
                data_req.data[j*si + i];
        this->map->cells[MAP_INDEX(this->map,oi+i,oj+j)].occ_dist = 0;
      }
    }

    oi += si;
    if(oi >= this->map->size_x)
    {
      oi = 0;
      oj += sj;
    }
  }

  // we're done with the map device now
  if(mapdriver->Unsubscribe(this->map_id) != 0)
    PLAYER_WARN("unable to unsubscribe from map device");

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int AMCLFiducial::Shutdown(void)
{
  this->driver->Unsubscribe(this->fiducial_id);
  this->driver = NULL;
  map_free(this->map);

  //deallocate fiducial map

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current laser reading
AMCLSensorData *AMCLFiducial::GetData(void)
{
  int i;
  size_t size;
  player_fiducial_data_t data;
  struct timeval timestamp;
  double r, b;
  AMCLFiducialData *ndata;

  // Get the laser device data.
  size = this->driver->GetData(this->fiducial_id, (void*) &data,
                               sizeof(data), &timestamp);
  if (size == 0)
    return NULL;
  if((timestamp.tv_sec == this->time.tv_sec) &&
     (timestamp.tv_usec == this->time.tv_usec))
    return NULL;

  double ta = (double) timestamp.tv_sec + ((double) timestamp.tv_usec) * 1e-6;
  double tb = (double) this->time.tv_sec + ((double) this->time.tv_usec) * 1e-6;
  if (ta - tb < 0.100)  // HACK
    return NULL;

  this->time = timestamp;

  ndata = new AMCLFiducialData;

  ndata->sensor = this;
  ndata->tsec = timestamp.tv_sec;
  ndata->tusec = timestamp.tv_usec;

  ndata->fiducial_count = ntohs(data.count);
  assert((size_t) ndata->fiducial_count < sizeof(ndata->fiducials) / sizeof(ndata->fiducials[0]));

  // Read and byteswap the range data
  for (i = 0; i < ndata->fiducial_count; i++)
  {
    double x = ((int16_t) ntohl(data.fiducials[i].pos[0]))/ 1000.0;
    double y = ((int16_t) ntohl(data.fiducials[i].pos[1]))/ 1000.0;

    r = hypot( y, x );
    b = atan2( y, x );

    ndata->fiducials[i][0] = r;
    ndata->fiducials[i][1] = b;
    ndata->fiducials[i][2] = ((double) ntohs( data.fiducials[i].id ));
  }

  return ndata;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the laser sensor model
bool AMCLFiducial::UpdateSensor(pf_t *pf, AMCLSensorData *data)
{
  AMCLFiducialData *ndata;

  ndata = (AMCLFiducialData*) data;

  // Apply the laser sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) SensorModel, data);

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Determine the probability for the given pose
double AMCLFiducial::SensorModel(AMCLFiducialData *data, pf_vector_t pose)
{
  AMCLFiducial *self;
  int i;
  double z, c, pz;
  double b, v, pb;
  double p;
  double map_range, map_bearing;
  double obs_range, obs_bearing;
  int obs_id;

  self = (AMCLFiducial*) data->sensor;

  // Take account of the laser pose relative to the robot
  pose = pf_vector_coord_add(self->laser_pose, pose);

  p = 1.0;

  for (i = 0; i < data->fiducial_count; i++)
  {
    obs_range = data->fiducials[i][0] + 0.05;
    obs_bearing = data->fiducials[i][1];
    obs_id = (int) data->fiducials[i][2];

    int id = -1;

    for( int j = 0; j < self->fmap->fiducial_count; j++ )
    {
      if( (int) data->fiducials[i][2] == (int) self->fmap->fiducials[j][2] )
      {
        id = j;
      }
    }

    if( id > -1 )
    {

      // Compute the range according to the map
      map_range = fiducial_map_calc_range( self->fmap, pose.v[0], pose.v[1],                               pose.v[2] + obs_bearing, self->range_max + 1.0, obs_id, id);
      map_bearing = fiducial_map_calc_bearing( self->fmap, pose.v[0], pose.v[1], pose.v[2], self->range_max + 1.0, obs_id, id );

      if (obs_range >= self->range_max && map_range >= self->range_max)
      {
        pz = 1.0;
        pb = 1.0;
      }
      else
      {
        // TODO: proper sensor model (using Kolmagorov?)
        // Simple gaussian model
        c = self->range_var;
        z = obs_range - map_range;
        pz = self->range_bad + (1 - self->range_bad) * exp(-(z*z)/(2*c*c));

        v = self->angle_var;
        b = obs_bearing - map_bearing;
        pb = self->angle_bad + (1 - self->angle_bad) * exp(-(b*b)/(2*v*v));

      }

      //gives the fiducial range greater weight
      p *= pz;

      //bearing a little less
      p *= pb;
      p *= pb;
      p *= pb;
      p *= pb;

    }
  }
  //printf("%e\n", p);
  //assert(p >= 0);

  return p;
}



#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLFiducial::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  this->fig = rtk_fig_create(canvas, robot_fig, 0);

  // Draw the laser map
  this->map_fig = rtk_fig_create(canvas, NULL, -50);
  map_draw_occ(this->map, this->map_fig);

  char x[3];
  x[2] = '\0';

  rtk_fig_color_rgb32( map_fig, 0xFF00FF );
  for( int i = 0; i < fmap->fiducial_count; i++ )
  {
	x[0] = '0' + (char) ((int) fmap->fiducials[i][2] / 10);
	x[1] = '0' + (char) ((int) fmap->fiducials[i][2] % 10);
     rtk_fig_ellipse(this->map_fig, fmap->fiducials[i][0], fmap->fiducials[i][1], 0, 0.1, 0.1, 1 );
	 rtk_fig_text( this->map_fig, fmap->fiducials[i][0], fmap->fiducials[i][1]+0.2, 0, x );

  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLFiducial::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_destroy(this->map_fig);
  rtk_fig_destroy(this->fig);
  this->map_fig = NULL;
  this->fig = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the laser values
void AMCLFiducial::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig, AMCLSensorData *data)
{
  int i;
  double r, b, ax, ay, bx, by;
  AMCLFiducialData *ndata;

  ndata = (AMCLFiducialData*) data;

  rtk_fig_clear(this->fig);

  // Draw the complete scan
  rtk_fig_color_rgb32(this->fig, 0x8080FF);
  char xx[3];
  xx[2] = '\0';
  for (i = 0; i < ndata->fiducial_count; i++ )
  {
    r = ndata->fiducials[i][0];
    b = ndata->fiducials[i][1];

    ax = 0;
    ay = 0;
    bx = ax + r * cos(b);
    by = ay + r * sin(b);

	xx[0] = '0' + (char) ((int) ndata->fiducials[i][2]/10);
	xx[1] = '0' + (char) ((int) ndata->fiducials[i][2]%10);
    rtk_fig_line(this->fig, ax, ay, bx, by);
	rtk_fig_text(this->fig, bx-0.2, by+0.2, 0, xx );
  }

  return;
}

#endif

double fiducial_map_calc_range( AMCLFiducialMap *fmap, double ox, double oy, double oa, double max_range, int id, int k)
{

		double r = hypot( fmap->fiducials[k][0] - ox, fmap->fiducials[k][1] - oy );

		double b = atan2( oy - fmap->fiducials[k][1], ox - fmap->fiducials[k][0] );
		if( fabs( b ) < acos( (2*r*r-(0.05*0.05))/(2.0*r*r)))
		{
			return r;
		}

  return max_range;
}

AMCLFiducialMap* fiducial_map_alloc(void)
{
 AMCLFiducialMap* map;
 map = new AMCLFiducialMap();
 map->fiducial_count = 0;

 if( !map )
 {
   PLAYER_ERROR( "map is undefined" );
 }

 return map;
}


double fiducial_map_calc_bearing( AMCLFiducialMap *fmap, double ox, double oy, double oa, double max_range, int id, int k)
{

  double b = atan2( fmap->fiducials[k][1]-oy, fmap->fiducials[k][0] -ox) - oa;
  if( b > M_PI  )
  {
    b -= 2*M_PI;
  }
  if( b < -M_PI  )
  {
    b += 2*M_PI;
  }

  return b;
}
#endif
