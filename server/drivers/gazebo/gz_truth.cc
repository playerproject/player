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
// Desc: Gazebo (simulator) truth driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_TRUTH
#warning "gz_truth not supported by libgazebo; skipping"
#else

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "error.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzTruth : public Driver
{
  // Constructor
  public: GzTruth(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzTruth();

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

  // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_truth_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzTruth_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzTruth(cf, section)));
}


// a driver registration function
void GzTruth_Register(DriverTable* table)
{
  table->AddDriver("gz_truth", GzTruth_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzTruth::GzTruth(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_TRUTH_CODE, PLAYER_ALL_MODE,
             sizeof(player_truth_data_t), 0, 10, 10)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_truth_alloc();
  
  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzTruth::~GzTruth()
{
  gz_truth_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzTruth::Setup()
{ 
  // Open the interface
  if (gz_truth_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzTruth::Shutdown()
{
  gz_truth_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzTruth::Update()
{
  player_truth_data_t data;
  struct timeval ts;
  
  gz_truth_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    data.px = htonl((int32_t) (1000 * this->iface->data->pos[0]));
    data.py = htonl((int32_t) (1000 * this->iface->data->pos[1]));
    data.pa = htonl((int32_t) (180 * this->iface->data->rot[2] / M_PI));
    
    this->PutData(&data, sizeof(data), &ts);
  }

  gz_truth_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzTruth::PutCommand(player_device_id_t id,
                         void* src, size_t len,
                         struct timeval* timestamp)
{  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzTruth::PutConfig(player_device_id_t id, void *client, 
                       void* src, size_t len,
                       struct timeval* timestamp)
{
  uint8_t subtype;

  subtype = ((uint8_t*) src)[0];
  switch (subtype)
  {
    case PLAYER_TRUTH_GET_POSE:
    {
      player_truth_pose_t rep;

      gz_truth_lock(this->iface, 1);
            
      rep.px = htonl((int32_t) (1000 * this->iface->data->pos[0]));
      rep.py = htonl((int32_t) (1000 * this->iface->data->pos[1]));
      rep.pa = htonl((int32_t) (180 * this->iface->data->rot[2] / M_PI));

      gz_truth_unlock(this->iface);

      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, (void*) &rep, sizeof(rep),NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
    case PLAYER_TRUTH_SET_POSE:
    {
      player_truth_pose_t *req = (player_truth_pose_t*) src;
      
      gz_truth_lock(this->iface, 1);

      this->iface->data->cmd_pos[0] = ((int32_t) ntohl(req->px)) / 1000.0;
      this->iface->data->cmd_pos[1] = ((int32_t) ntohl(req->py)) / 1000.0;
      this->iface->data->cmd_pos[2] = this->iface->data->pos[2];

      this->iface->data->cmd_rot[0] = this->iface->data->rot[0];
      this->iface->data->cmd_rot[1] = this->iface->data->rot[1];
      this->iface->data->cmd_rot[2] = ((int32_t) ntohl(req->pa)) / 180.0 * M_PI;

      this->iface->data->cmd_new = 1;

      gz_truth_unlock(this->iface);

      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }
  return 0;
}

#endif
