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
// Desc: Gazebo (simulator) gps driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_GPS
#warning "gz_gps not supported by libgazebo; skipping"
#else

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzGps : public Driver
{
  // Constructor
  public: GzGps(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzGps();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Commands
  public: virtual void PutCommand(player_device_id_t id, void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t id, player_device_id_t* device, void* client, void* data, size_t len);

  // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_gps_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzGps_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzGps(cf, section)));
}


// a driver registration function
void GzGps_Register(DriverTable* table)
{
  table->AddDriver("gz_gps", GzGps_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzGps::GzGps(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_GPS_CODE, PLAYER_ALL_MODE,
             sizeof(player_gps_data_t), 0, 10, 10)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_gps_alloc();
  
  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzGps::~GzGps()
{
  gz_gps_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzGps::Setup()
{ 
  // Open the interface
  if (gz_gps_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzGps::Shutdown()
{
  gz_gps_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzGps::Update()
{
  player_gps_data_t data;
  uint32_t tsec, tusec;
  
  gz_gps_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    tsec = (int) (this->iface->data->time);
    tusec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    data.latitude = htonl((int32_t) (216000 * this->iface->data->latitude));
    data.longitude = htonl((int32_t) (216000 * this->iface->data->longitude));
    data.altitude = htonl((int32_t) (1000 * this->iface->data->altitude));
    data.utm_e = htonl((int32_t) (100 * this->iface->data->utm_e));
    data.utm_n = htonl((int32_t) (100 * this->iface->data->utm_n));
    data.num_sats = this->iface->data->satellites;
    data.quality = this->iface->data->quality;
    data.hdop = htons((uint32_t) (int32_t) (10 * this->iface->data->hdop));
    data.err_horz = htonl((uint32_t) (int32_t) (1000 * this->iface->data->err_horz));
    data.err_vert = htonl((uint32_t) (int32_t) (1000 * this->iface->data->err_vert));
    
    this->PutData(&data, sizeof(data), tsec, tusec);
  }

  gz_gps_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzGps::PutCommand(player_device_id_t id, void* client, unsigned char* src, size_t len)
{  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzGps::PutConfig(player_device_id_t id, player_device_id_t* device, void* client, void* data, size_t len)
{
  uint8_t subtype;

  subtype = ((uint8_t*) data)[0];
  switch (subtype)
  {
    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }
  return 0;
}

#endif
