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
// Desc: Gazebo (simulator) sonar driver
// Author: Carle Cote (Laborius - Universite de Sherbrooke - Sherbrooke, Quebec, Canada)
// Date: 23 february 2004
//
// Modification
// By:                     Description                            Date
// Victor Tran             implementation to take information     March 23, 2004
//                         and the data in Update() and
//                         HandleGetGeom()
//
// CVS:
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_SONAR
#warning "gz_sonar not supported by libgazebo; skipping"
#else

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <sys/types.h>
#include <netinet/in.h>

#include "player.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Gazebo Sonar driver
class GzSonar : public Driver
{
  // Constructor
  public: GzSonar(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzSonar();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Commands
  public: virtual void PutCommand(player_device_id_t id,
                                  void* src, size_t len,
                                  struct timeval* timestamp);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t id, void *client, 
                                void* src, size_t len,
                                struct timeval* timestamp);

  // Handle geometry requests
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Handle sonar configuration
  private: void HandleSonarPower(void *client, void *req, int reqlen);

  // Gazebo id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_sonar_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzSonar_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzSonar(cf, section)));
}


// a driver registration function
void GzSonar_Register(DriverTable* table)
{
  table->AddDriver("gz_sonar", GzSonar_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzSonar::GzSonar(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_SONAR_CODE, PLAYER_ALL_MODE,
             sizeof(player_sonar_data_t), 0, 10, 10)
{
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Create an interface
  this->iface = gz_sonar_alloc();

  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzSonar::~GzSonar()
{
  gz_sonar_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzSonar::Setup()
{
  // Open the interface
  if (gz_sonar_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzSonar::Shutdown()
{
  gz_sonar_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzSonar::Update()
{
  player_sonar_data_t data;
  struct timeval ts;

  gz_sonar_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;

    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    data.range_count = htons(this->iface->data->sonar_count);
    for (int i = 0; i < this->iface->data->sonar_count; i++)
      data.ranges[i] = htons((short) (int) (this->iface->data->sonar_ranges[i] * 1000.0));

    this->PutData(&data, sizeof(data), &ts);
  }

  gz_sonar_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzSonar::PutCommand(player_device_id_t id,
                          void* src, size_t len,
                          struct timeval* timestamp)
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzSonar::PutConfig(player_device_id_t id, void *client, 
                        void* src, size_t len,
                        struct timeval* timestamp)
{
  switch (((char*) src)[0])
  {
    case PLAYER_SONAR_GET_GEOM_REQ:
      HandleGetGeom(client, src, len);
      break;

    case PLAYER_SONAR_POWER_REQ:
      HandleSonarPower(client, src, len);
      break;

    default:
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void GzSonar::HandleGetGeom(void *client, void *req, int reqlen)
{
  player_sonar_geom_t geom;

  geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;

  gz_sonar_lock(this->iface, 1);

  geom.pose_count = htons(this->iface->data->sonar_count);

  //the position of valid sonar
  for (int i = 0; i < this->iface->data->sonar_count; i++)
  {
    geom.poses[i][0] = htons((short) (int) (this->iface->data->sonar_pos[i][0] * 1000.0));
    geom.poses[i][1] = htons((short) (int) (this->iface->data->sonar_pos[i][1] * 1000.0));
    geom.poses[i][2] = htons((short) (int) (this->iface->data->sonar_rot[i][2] * 180 / M_PI));
  }
  gz_sonar_unlock(this->iface);

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &geom, sizeof(geom),NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle sonar power 
void GzSonar::HandleSonarPower(void *client, void *req, int reqlen)
{
  player_sonar_power_config_t *power;
  
  assert((size_t) reqlen >= sizeof(player_sonar_power_config_t));
  power = (player_sonar_power_config_t*) req;

  gz_sonar_lock(this->iface, 1);

  // TODO
  
  gz_sonar_unlock(this->iface);

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}

#endif
