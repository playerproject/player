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
// Desc: Gazebo (simulator) gripper driver
// Author: Carle Cote (Laborius - Universite de Sherbrooke - Sherbrooke, Quebec, Canada)
// Date: 23 february 2004
// CVS: 
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup player_driver_gz_gripper gz_gripper
 * @brief Gazebo gripper

@todo This driver is currently disabled because it needs to be updated to
the Player 2.0 API.

*/
/** @} */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_GRIPPER
#warning "gz_gripper not supported by libgazebo; skipping"
#else

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

//#include <iostream>


// Gazebo Gripper driver
class GzGripper : public Driver
{
  // Constructor
  public: GzGripper(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzGripper();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  public: int ProcessMessage( MessageQueue *resp_queue, 
                              player_msghdr *hdr, 
                              void *data);

  // Gazebo id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_gripper_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzGripper_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzGripper(cf, section)));
}


// a driver registration function
void GzGripper_Register(DriverTable* table)
{
  table->AddDriver("gz_gripper", GzGripper_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzGripper::GzGripper(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_GRIPPER_CODE)
{
    // Get the globally defined Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Create an interface
  this->iface = gz_gripper_alloc();

  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzGripper::~GzGripper()
{
  gz_gripper_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzGripper::Setup()
{
  // Open the interface
  if (gz_gripper_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  // Add ourselves to the update list
  GzClient::AddDriver(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzGripper::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_gripper_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzGripper::Update()
{
  player_gripper_data_t data;
  struct timeval ts;

  gz_gripper_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    // break beams are now implemented
    data.beams = 0;
  
    data.beams |= this->iface->data->grip_limit_reach ? 0x01 : 0x00;
    data.beams |= this->iface->data->lift_limit_reach ? 0x02 : 0x00;
    data.beams |= this->iface->data->outer_beam_obstruct ? 0x04 : 0x00;
    data.beams |= this->iface->data->inner_beam_obstruct ? 0x08 : 0x00;
    data.beams |= this->iface->data->left_paddle_open ? 0x10 : 0x00;
    data.beams |= this->iface->data->right_paddle_open ? 0x20 : 0x00;  
    
    // set the proper bits
    data.state = 0;
    data.state |= this->iface->data->paddles_opened ? 0x01 : 0x00;
    data.state |= this->iface->data->paddles_closed ? 0x02 : 0x00;
    data.state |= this->iface->data->paddles_moving ? 0x04 : 0x00;
    data.state |= this->iface->data->paddles_error ? 0x08 : 0x00;
    data.state |= this->iface->data->lift_up ? 0x10 : 0x00;
    data.state |= this->iface->data->lift_down ? 0x20 : 0x00;
    data.state |= this->iface->data->lift_moving ? 0x40 : 0x00;
    data.state |= this->iface->data->lift_error ? 0x80 : 0x00;
    
    this->Publish( this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_GRIPPER_DATA_STATE,
                   (void*)&data, sizeof(data), &this->datatime );
 
  }

  gz_gripper_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process Messages
int GzGripper::ProcessMessage( MessageQueue *resp_queue, 
                                  player_msghdr *hdr, 
                                  void *data)
{
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_STATE, this->device_addr))
  {
    player_gripper_cmd_t *cmd;

    assert(hdr->size >= sizeof(player_gripper_cmd_t));
    cmd = (player_gripper_cmd_t*) data;

    gz_gripper_lock(this->iface, 1);

    this->iface->data->cmd = (int)(cmd->cmd);

    gz_gripper_unlock(this->iface);
  }
    
  return 0;
}
#endif
