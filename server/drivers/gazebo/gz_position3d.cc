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
// Desc: Gazebo (simulator) position3d driver
// Author: Pranav Srivastava
// Date: 28 Feb 2004
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
#include <stdlib.h>       // for atoi(3)
#include <sys/types.h>
#include <netinet/in.h>

#include "player.h"
#include "error.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzPosition3d : public Driver
{
  // Constructor
  public: GzPosition3d(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzPosition3d();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  public: virtual int ProcessMessage( MessageQueue *resp_queue, 
                                      player_msghdr *hdr, 
                                      void *data);
 
  // Handle geometry requests
  private: void HandleGetGeom(MessageQueue *resp_queue, 
                              player_msghdr *hdr, 
                              void *data);

  // Handle motor configuration
  private: void HandleMotorPower(MessageQueue *resp_queue, 
                                 player_msghdr *hdr, 
                                 void *data);

  // Gazebo id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_position_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzPosition3d_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzPosition3d(cf, section)));
}


// a driver registration function
void GzPosition3d_Register(DriverTable* table)
{
  table->AddDriver("gz_position3d", GzPosition3d_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzPosition3d::GzPosition3d(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_POSITION3D_CODE)
{
    // Get the globally defined Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Create an interface
  this->iface = gz_position_alloc();

  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzPosition3d::~GzPosition3d()
{
  gz_position_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzPosition3d::Setup()
{
  // Open the interface
  if (gz_position_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  // Add ourselves to the update list
  GzClient::AddDriver(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzPosition3d::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_position_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzPosition3d::Update()
{
  player_position3d_data_t data;
  struct timeval ts;

  gz_position_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);
  
    data.pos[0] = this->iface->data->pos[0];
    data.pos[1] = this->iface->data->pos[1];
    data.pos[2] = this->iface->data->pos[2];

    data.pos[3] = this->iface->data->rot[0];
    data.pos[4] = this->iface->data->rot[1];
    data.pos[5] = this->iface->data->rot[2];

    data.vel[0] = this->iface->data->vel_pos[0];
    data.vel[1] = this->iface->data->vel_pos[1];
    data.vel[2] = this->iface->data->vel_pos[2];

    data.vel[3] = this->iface->data->vel_rot[0];
    data.vel[4] = this->iface->data->vel_rot[1];
    data.vel[5] = this->iface->data->vel_rot[2];

    data.stall = (uint8_t) this->iface->data->stall;

    this->Publish( this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_POSITION3D_DATA_STATE, 
                   (void*)&data, sizeof(data), &this->datatime );
 
  }

  gz_position_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process Messages
int GzPosition3d::ProcessMessage( MessageQueue *resp_queue, 
                                  player_msghdr *hdr, 
                                  void *data)
{
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION3D_SET_ODOM,this->device_addr))
  {
    player_position3d_cmd_t *cmd;

    assert(hdr->size >= sizeof(player_position3d_cmd_t));
    cmd = (player_position3d_cmd_t*) data;

    gz_position_lock(this->iface, 1);
    this->iface->data->cmd_vel_pos[0] = cmd->vel[0];
    this->iface->data->cmd_vel_pos[1] = cmd->vel[1];
    this->iface->data->cmd_vel_pos[2] = cmd->vel[2];
    this->iface->data->cmd_vel_rot[0] = cmd->vel[3];
    this->iface->data->cmd_vel_rot[1] = cmd->vel[4];
    this->iface->data->cmd_vel_rot[2] = cmd->vel[5];
    gz_position_unlock(this->iface);
  }

  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION3D_GET_GEOM,this->device_addr))
  {
    this->HandleGetGeom(resp_queue, hdr, data);
  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION3D_MOTOR_POWER,this->device_addr))
  {
    this->HandleMotorPower(resp_queue, hdr, data);
  }
   
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void GzPosition3d::HandleGetGeom(MessageQueue *resp_queue, 
                                  player_msghdr *hdr, 
                                  void *data)
{
  player_position3d_geom_t geom;

  // TODO: get correct dimensions; there are for the P2AT
  // i think this is only for the playerv client .. not really a necessity??  
  geom.pose[0] = 0;
  geom.pose[1] = 0;
  geom.pose[2] = 0;
  geom.size[0] = 0.53;
  geom.size[1] = 0.38;

  this->Publish(this->device_addr, resp_queue,
                PLAYER_MSGTYPE_RESP_ACK, 
                PLAYER_POSITION3D_GET_GEOM, 
                &geom, sizeof(geom), NULL);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle motor power 
void GzPosition3d::HandleMotorPower(MessageQueue *resp_queue, 
                                  player_msghdr *hdr, 
                                  void *data)
{
  player_position3d_power_config_t *power;
  
  assert((size_t) hdr->size >= sizeof(player_position3d_power_config_t));
  power = (player_position3d_power_config_t*) data;

  gz_position_lock(this->iface, 1);
  this->iface->data->cmd_enable_motors = power->state;
  gz_position_unlock(this->iface);

  this->Publish(this->device_addr, resp_queue,
                PLAYER_MSGTYPE_RESP_ACK, 
                PLAYER_POSITION3D_MOTOR_POWER,
                power, sizeof(power), NULL);
  return;
}
