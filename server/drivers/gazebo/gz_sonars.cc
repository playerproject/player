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
// Desc: Gazebo (simulator) sonars driver
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

#ifndef INCLUDE_GAZEBO_SONARS
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
#include "device.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Gazebo Sonars driver
class GzSonars : public CDevice
{
  // Constructor
  public: GzSonars(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzSonars();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t* device, void* client,
                                void* req, size_t reqlen);

  // Handle geometry requests
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Handle sonars configuration
  private: void HandleSonarsPower(void *client, void *req, int reqlen);

  // Gazebo id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_sonars_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
CDevice* GzSonars_Init(char* interface, ConfigFile* cf, int section)
{
  //printf("GzSonars_Init\n");
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_SONAR_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_sonars\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzSonars(interface, cf, section)));
}


// a driver registration function
void GzSonars_Register(DriverTable* table)
{
  //printf("GzSonars_Register\n");
  table->AddDriver("gz_sonars", PLAYER_ALL_MODE, GzSonars_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzSonars::GzSonars(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_sonar_data_t), 0, 10, 10)
{
  //printf("GzSonars::GzSonars\n");
    // Get the globally defined Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Create an interface
  this->iface = gz_sonars_alloc();

  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzSonars::~GzSonars()
{
  //printf("GzSonars::~GzSonars\n");
  gz_sonars_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzSonars::Setup()
{
  //printf("GzSonars::Setup\n");
  // Open the interface
  if (gz_sonars_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzSonars::Shutdown()
{
  //printf("GzSonars::Shutdown\n");
  gz_sonars_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzSonars::Update()
{
  //printf("GzSonars::Update\n");
  player_sonar_data_t data;
  uint32_t tsec, tusec;

  gz_sonars_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;

    tsec = (int) (this->iface->data->time);
    tusec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    //Modify by Victor Tran
    //---> Add
    //the count of valid sonars
    data.range_count = htons(this->iface->data->range_count);

    //the range of valid sonars
    for (int i = 0; i < this->iface->data->range_count; i++)
    {
      data.ranges[i] = htons(this->iface->data->ranges[i]);
    }
    // <--- end Add
    this->PutData(&data, sizeof(data), tsec, tusec);
  }

  gz_sonars_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzSonars::PutCommand(void* client, unsigned char* src, size_t len)
{
  //printf("GzSonars::PutCommand\n");
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzSonars::PutConfig(player_device_id_t* device, void* client, void* req, size_t req_len)
{
  //printf("GzSonars::PutConfig\n");
  switch (((char*) req)[0])
  {
    case PLAYER_SONAR_GET_GEOM_REQ:
      HandleGetGeom(client, req, req_len);
      break;

    case PLAYER_SONAR_POWER_REQ:
      HandleSonarsPower(client, req, req_len);
      break;

    default:
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void GzSonars::HandleGetGeom(void *client, void *req, int reqlen)
{
  //printf("GzSonars::HandleGetGeom\n");
  player_sonar_geom_t geom;

  geom.subtype = PLAYER_SONAR_GET_GEOM_REQ;

  //Modify by Victor Tran
  //---> Add
  //the count of valid sonars
  gz_sonars_lock(this->iface, 1);
  geom.pose_count = htons(this->iface->data->pose_count);

  //the position of valid sonars
  for (int i = 0; i < this->iface->data->pose_count; i++)
  {
    geom.poses[i][0] = htons(this->iface->data->poses[i][0]);
    geom.poses[i][1] = htons(this->iface->data->poses[i][1]);
    geom.poses[i][2] = htons(this->iface->data->poses[i][2]);
  }
  gz_sonars_unlock(this->iface);
  //<--- end Add
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle sonars power 
void GzSonars::HandleSonarsPower(void *client, void *req, int reqlen)
{
  //printf("GzSonars::HandleSonarsPower\n");
  player_sonar_power_config_t *power;
  
  assert((size_t) reqlen >= sizeof(player_sonar_power_config_t));
  power = (player_sonar_power_config_t*) req;

  gz_sonars_lock(this->iface, 1);
  //this->iface->data->cmd_enable_motors = power->value;
  gz_sonars_unlock(this->iface);

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}

#endif
