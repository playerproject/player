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
// Desc: Gazebo (simulator) laser driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "device.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzLaser : public CDevice
{
  // Constructor
  public: GzLaser(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzLaser();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t* device, void* client, void* data, size_t len);

  // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_laser_t *iface;

  // Timestamp on last data update
  private: uint32_t tsec, tusec;
};


// Initialization function
CDevice* GzLaser_Init(char* interface, ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_LASER_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_laser\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzLaser(interface, cf, section)));
}


// a driver registration function
void GzLaser_Register(DriverTable* table)
{
  table->AddDriver("gz_laser", PLAYER_ALL_MODE, GzLaser_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzLaser::GzLaser(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_laser_data_t), 0, 10, 10)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_laser_alloc();
  
  this->tsec = this->tusec = 0;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzLaser::~GzLaser()
{
  gz_laser_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzLaser::Setup()
{ 
  // Open the interface
  if (gz_laser_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzLaser::Shutdown()
{
  gz_laser_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t GzLaser::GetData(void* client, unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int i;
  player_laser_data_t data;
  uint32_t tsec, tusec;
  double range_res;
  
  gz_laser_lock(this->iface, 1);

  // Pick the rage resolution to use (1, 10, 100)
  if (this->iface->data->max_range < 8.192)
    range_res = 1.0;
  else if (this->iface->data->max_range < 81.92)
    range_res = 10.0;
  else
    range_res = 100.0;
  
  data.min_angle = htons((int) (this->iface->data->min_angle * 100 * 180 / M_PI));
  data.max_angle = htons((int) (this->iface->data->max_angle * 100 * 180 / M_PI));
  data.resolution = htons((int) (this->iface->data->res_angle * 100 * 180 / M_PI));
  data.range_res = htons((int) range_res);
  data.range_count = htons((int) (this->iface->data->range_count));
  
  for (i = 0; i < this->iface->data->range_count; i++)
  {
    data.ranges[i] = htons((int) (this->iface->data->ranges[i] * 1000 / range_res));
    data.intensity[i] = (uint8_t) (int) this->iface->data->intensity[i];
  }

  assert(len >= sizeof(data));
  memcpy(dest, &data, sizeof(data));

  tsec = (int) (this->iface->data->time);
  tusec = (int) (fmod(this->iface->data->time, 1) * 1e6);

  gz_laser_unlock(this->iface);

  // signal that new data is available
  if (tsec != this->tsec || tusec != this->tusec)
    DataAvailable();
  this->tsec = tsec;
  this->tusec = tusec;

  if (timestamp_sec)
    *timestamp_sec = tsec;
  if (timestamp_usec)
    *timestamp_usec = tusec;

  return sizeof(data);
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzLaser::PutCommand(void* client, unsigned char* src, size_t len)
{  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzLaser::PutConfig(player_device_id_t* device, void* client, void* data, size_t len)
{
  uint8_t subtype;

  subtype = ((uint8_t*) data)[0];
  switch (subtype)
  {
    case PLAYER_LASER_GET_GEOM:
    {
      player_laser_geom_t rep;

      // TODO: get geometry from somewhere
      rep.subtype = PLAYER_LASER_GET_GEOM;
      rep.pose[0] = htons((int) (0.0));
      rep.pose[1] = htons((int) (0.0));
      rep.pose[2] = htons((int) (0.0));
      rep.size[0] = htons((int) (0.0));
      rep.size[1] = htons((int) (0.0));

      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &rep, sizeof(rep)) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }

    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }
  return 0;
}
