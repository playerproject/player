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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: AMCL laser routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define PLAYER_ENABLE_MSG 1

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include "devicetable.h"
#include "amcl_laser.h"

extern int global_playerport; // used to gen. useful output & debug


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLLaser::AMCLLaser()
{
  this->device = NULL;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load laser settings
int AMCLLaser::Load(ConfigFile* cf, int section)
{
  // Device stuff
  this->laser_index = cf->ReadInt(section, "laser_index", -1);

  // Get the map settings
  this->map_index = cf->ReadInt(section, "laser_map_index", -1);
  
  this->laser_pose.v[0] = cf->ReadTupleLength(section, "laser_pose", 0, 0);
  this->laser_pose.v[1] = cf->ReadTupleLength(section, "laser_pose", 1, 0);
  this->laser_pose.v[2] = cf->ReadTupleAngle(section, "laser_pose", 2, 0);

  this->max_beams = cf->ReadInt(section, "laser_max_beams", 6);
  this->range_max = cf->ReadLength(section, "laser_range_max", 8.192);
  this->range_var = cf->ReadLength(section, "laser_range_var", 0.10);
  this->range_bad = cf->ReadFloat(section, "laser_range_bad", 0.10);

  this->tsec = 0;
  this->tusec = 0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLLaser::Unload(void)
{
  //laser_free(this->model);
  //this->model = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int AMCLLaser::Setup(void)
{
  //uint8_t req;
  //uint16_t reptype;
  //player_laser_geom_t geom;
  //struct timeval tv;
  player_device_id_t id;

  if(this->SetupMap() < 0)
  {
    PLAYER_ERROR("failed to get laser map");
    return(-1);
  }

  // Subscribe to the Laser device
  id.port = global_playerport;
  id.code = PLAYER_LASER_CODE;
  id.index = this->laser_index;

  this->device = deviceTable->GetDevice(id);
  if (!this->device)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return -1;
  }
  if (this->device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return -1;
  }

  // TODO: use laser geometry request?

  /*
  // Get the laser geometry
  req = PLAYER_LASER_GET_GEOM;
  if (this->device->Request(&id, this, &req, 1, &reptype, &tv, &geom, sizeof(geom)) < 0)
  {
    PLAYER_ERROR("unable to get laser geometry");
    return -1;
  }

  // Set the laser pose relative to the robot
  this->laser_pose.v[0] = ((int16_t) ntohl(geom.pose[0])) / 1000.0;
  this->laser_pose.v[1] = ((int16_t) ntohl(geom.pose[1])) / 1000.0;
  this->laser_pose.v[2] = ((int16_t) ntohl(geom.pose[2])) * M_PI / 180.0;
  */

  return 0;
}

int
AMCLLaser::SetupMap(void)
{
  player_device_id_t map_id;
  CDevice* mapdevice;

  // Subscribe to the map device
  map_id.port = global_playerport;
  map_id.code = PLAYER_MAP_CODE;
  map_id.index = this->map_index;

  if(!(mapdevice = deviceTable->GetDevice(map_id)))
  {
    PLAYER_ERROR("unable to locate suitable map device");
    return -1;
  }
  if(mapdevice->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to map device");
    return -1;
  }

  // Create the map
  this->map = map_alloc();
  //PLAYER_MSG1("loading map file [%s]", map_filename);
  PLAYER_MSG1("reading map from map:%d", this->map_index);
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
  info.subtype = PLAYER_MAP_GET_INFO_REQ;
  if((replen = mapdevice->Request(&map_id, this, &info, 
                                  sizeof(info.subtype), &reptype, 
                                  &ts, &info, sizeof(info))) == 0)
  {
    PLAYER_ERROR("failed to get map info");
    return(-1);
  }
  
  // copy in the map info
  this->map->origin_x = this->map->origin_y = 0.0;
  this->map->scale = 1/(ntohl(info.scale) / 1e3);
  this->map->size_x = ntohl(info.width);
  this->map->size_y = ntohl(info.height);

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

  data_req.subtype = PLAYER_MAP_GET_DATA_REQ;
  
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

    if((replen = mapdevice->Request(&map_id, this, &data_req, reqlen,
                                    &reptype, &ts, &data_req, 
                                    sizeof(data_req))) == 0)
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
  if(mapdevice->Unsubscribe(this) != 0)
    PLAYER_WARN("unable to unsubscribe from map device");

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int AMCLLaser::Shutdown(void)
{  
  this->device->Unsubscribe(this);
  this->device = NULL;
  map_free(this->map);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current laser reading
AMCLSensorData *AMCLLaser::GetData(void)
{
  int i;
  size_t size;
  player_laser_data_t data;
  uint32_t tsec, tusec;
  double r, b, db, res;
  AMCLLaserData *ndata;

  // Get the laser device data.
  size = this->device->GetData(this, (uint8_t*) &data, sizeof(data), &tsec, &tusec);
  if (size == 0)
    return NULL;
  if (tsec == this->tsec && tusec == this->tusec)
    return NULL;

  double ta = (double) tsec + ((double) tusec) * 1e-6;
  double tb = (double) this->tsec + ((double) this->tusec) * 1e-6;  
  if (ta - tb < 0.100)  // HACK
    return NULL;

  this->tsec = tsec;
  this->tusec = tusec;
  
  b = ((int16_t) ntohs(data.min_angle)) / 100.0 * M_PI / 180.0;
  db = ((int16_t) ntohs(data.resolution)) / 100.0 * M_PI / 180.0;
  res = ((int16_t) ntohs(data.range_res));
  
  ndata = new AMCLLaserData;

  ndata->sensor = this;
  ndata->tsec = tsec;
  ndata->tusec = tusec;
  
  ndata->range_count = ntohs(data.range_count);
  assert((size_t) ndata->range_count < sizeof(ndata->ranges) / sizeof(ndata->ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < ndata->range_count; i++)
  {
    r = ((uint16_t) ntohs(data.ranges[i])) * res / 1000.0;
    ndata->ranges[i][0] = r;
    ndata->ranges[i][1] = b;
    b += db;
  }
  
  return ndata;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the laser sensor model
bool AMCLLaser::UpdateSensor(pf_t *pf, AMCLSensorData *data)
{
  AMCLLaserData *ndata;
  
  ndata = (AMCLLaserData*) data;
  if (this->max_beams < 2)
    return false;

  // Apply the laser sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) SensorModel, data);
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Determine the probability for the given pose
double AMCLLaser::SensorModel(AMCLLaserData *data, pf_vector_t pose)
{
  AMCLLaser *self;
  int i, step;
  double z, c, pz;
  double p;
  double map_range;
  double obs_range, obs_bearing;
  
  self = (AMCLLaser*) data->sensor;

  // Take account of the laser pose relative to the robot
  pose = pf_vector_coord_add(self->laser_pose, pose);

  p = 1.0;

  step = (data->range_count - 1) / (self->max_beams - 1);
  for (i = 0; i < data->range_count; i += step)
  {
    obs_range = data->ranges[i][0];
    obs_bearing = data->ranges[i][1];

    // Compute the range according to the map
    map_range = map_calc_range(self->map, pose.v[0], pose.v[1],
                               pose.v[2] + obs_bearing, self->range_max + 1.0);

    if (obs_range >= self->range_max && map_range >= self->range_max)
    {
      pz = 1.0;
    }
    else
    {
      // TODO: proper sensor model (using Kolmagorov?)
      // Simple gaussian model
      c = self->range_var;
      z = obs_range - map_range;
      pz = self->range_bad + (1 - self->range_bad) * exp(-(z * z) / (2 * c * c));
    }

    /*
    if (obs->range >= 8.0 && map_range >= 8.0)
      p *= 1.0;
    else if (obs->range >= 8.0 && map_range < 8.0)
      p *= self->range_bad;
    else if (obs->range < 8.0 && map_range >= 8.0)
      p *= self->range_bad;
    else
      p *= laser_sensor_prob(self, obs->range, map_range);
    */
    
    p *= pz;
  }

  //printf("%e\n", p);
  //assert(p >= 0);
  
  return p;
}



#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLLaser::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{  
  this->fig = rtk_fig_create(canvas, robot_fig, 0);

  // Draw the laser map
  this->map_fig = rtk_fig_create(canvas, NULL, -50);
  map_draw_occ(this->map, this->map_fig);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLLaser::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_destroy(this->map_fig);
  rtk_fig_destroy(this->fig);
  this->map_fig = NULL;
  this->fig = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the laser values
void AMCLLaser::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig, AMCLSensorData *data)
{
  int i, step;
  double r, b, ax, ay, bx, by;
  AMCLLaserData *ndata;

  ndata = (AMCLLaserData*) data;
  
  rtk_fig_clear(this->fig);

  // Draw the complete scan
  rtk_fig_color_rgb32(this->fig, 0x8080FF);
  for (i = 0; i < ndata->range_count; i++)
  {
    r = ndata->ranges[i][0];
    b = ndata->ranges[i][1];

    ax = 0;
    ay = 0;
    bx = ax + r * cos(b);
    by = ay + r * sin(b);

    rtk_fig_line(this->fig, ax, ay, bx, by);
  }

  if (this->max_beams < 2)
    return;

  // Draw the significant part of the scan
  step = (ndata->range_count - 1) / (this->max_beams - 1);
  for (i = 0; i < ndata->range_count; i += step)
  {
    r = ndata->ranges[i][0];
    b = ndata->ranges[i][1];
    //m = map_calc_range(this->map, pose.v[0], pose.v[1], pose.v[2] + b, 8.0);

    ax = 0;
    ay = 0;

    bx = ax + r * cos(b);
    by = ay + r * sin(b);
    rtk_fig_color_rgb32(this->fig, 0xFF0000);
    rtk_fig_line(this->fig, ax, ay, bx, by);
  }

  return;
}

#endif



