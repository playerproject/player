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
// Desc: AMCL sonar routines
// Author: Oscar Gerelli, Dario Lodi Rizzini
// Date: 22 Jun 2005
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define PLAYER_ENABLE_MSG 1

#include <sys/types.h> // required by Darwin
#include <netinet/in.h>
#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sstream>
#include "error.h"
#include "devicetable.h"
#include "amcl_sonar.h"

extern int global_playerport; // used to gen. useful output & debug


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLSonar::AMCLSonar(player_device_id_t id)
{
  this->driver = NULL;
  this->sonar_id = id;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load sonar settings
int AMCLSonar::Load(ConfigFile* cf, int section)
{
  // Get the map settings.  Don't error check here; we'll do it later, in
  // SetupMap().
  cf->ReadDeviceId(&(this->map_id), section, "requires",
                   PLAYER_MAP_CODE, -1, "sonar");
  

  this->scount = cf->ReadInt(section, "scount", 0);
  assert(this->scount > 0);

  sonar_pose = new pf_vector_t[scount];

  for(int i=0; i < scount; i++) {
     std::stringstream sonar;
     sonar << "spose[" << i << "]";
     this->sonar_pose[i].v[0] = cf->ReadTupleLength(section, sonar.str().c_str(), 0, 0);
     this->sonar_pose[i].v[1] = cf->ReadTupleLength(section, sonar.str().c_str(), 1, 0);
     this->sonar_pose[i].v[2] = cf->ReadTupleAngle(section, sonar.str().c_str(), 2, 0);
  }

  this->range_max = cf->ReadLength(section, "sonar_range_max", 4.0);
  this->range_var = cf->ReadLength(section, "sonar_range_var", 0.50);
  this->range_bad = cf->ReadFloat(section, "sonar_range_bad", 0.30);

  this->time.tv_sec = 0;
  this->time.tv_usec = 0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLSonar::Unload(void)
{
 
  delete[] sonar_pose;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the sonar
int AMCLSonar::Setup(void)
{

  if(this->SetupMap() < 0)
  {
    PLAYER_ERROR("failed to get sonar map");
    return(-1);
  }

  // Subscribe to the Sonar device
  this->driver = deviceTable->GetDriver(this->sonar_id);
  if (!this->driver)
  {
    PLAYER_ERROR("unable to locate suitable sonar device");
    return -1;
  }
  if (this->driver->Subscribe(this->sonar_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to sonar device");
    return -1;
  }

  return 0;
}

// TODO: should Unsubscribe from the map on error returns in the function
int
AMCLSonar::SetupMap(void)
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
  info.subtype = PLAYER_MAP_GET_INFO_REQ;
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
// Shut down the sonar
int AMCLSonar::Shutdown(void)
{
  this->driver->Unsubscribe(this->sonar_id);
  this->driver = NULL;
  map_free(this->map);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current sonar reading
AMCLSensorData *AMCLSonar::GetData(void)
{

  int i;
  size_t size;
  player_sonar_data_t data;
  struct timeval timestamp;
  double r;
  AMCLSonarData *ndata;

  assert(driver != NULL);

  // Get the sonar device data.
  size = this->driver->GetData(this->sonar_id, (void*) &data, 
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
  
  ndata = new AMCLSonarData;

  ndata->sensor = this;
  ndata->tsec = timestamp.tv_sec;
  ndata->tusec = timestamp.tv_usec;
  
  ndata->range_count = ntohs(data.range_count);
  assert((size_t) ndata->range_count * sizeof(ndata->ranges[0]) <= sizeof(ndata->ranges));

  assert(ndata->range_count == scount);

  // Read and byteswap the range data
  for (i = 0; i < ndata->range_count; i++)
  {
    r = ((uint16_t) ntohs(data.ranges[i])) / 1000.0;
    ndata->ranges[i][0] = r;
    ndata->ranges[i][1] = sonar_pose[i].v[2];
  }
  
  return ndata;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the sonar sensor model
bool AMCLSonar::UpdateSensor(pf_t *pf, AMCLSensorData *data)
{
  AMCLSonarData *ndata;
  
  ndata = (AMCLSonarData*) data;

  // Apply the sonar sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) SensorModel, data);

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Determine the probability for the given pose
double AMCLSonar::SensorModel(AMCLSonarData *data, pf_vector_t robotPose)
{
  AMCLSonar *self;
  int i;
  double z, c, pz;
  double p;
  double map_range;
  double obs_range, obs_bearing;
  pf_vector_t pose;
  
  self = (AMCLSonar*) data->sensor;


  p = 1.0;

  for (i = 0; i < data->range_count; i++)
  {
     // Take account of the sonar pose relative to the robot
    pose = pf_vector_coord_add(self->sonar_pose[i], robotPose);

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

    p *= pz;
  }

  //printf("%e\n", p);
  //assert(p >= 0);
  
  return p;

}


