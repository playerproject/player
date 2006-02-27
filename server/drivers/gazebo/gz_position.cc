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
// Desc: Gazebo (simulator) position driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup player_driver_gz_position gz_position
 * @brief Gazebo position

@todo This driver is currently disabled because it needs to be updated to
the Player 2.0 API.

*/
/** @} */

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

/*#include "player.h"
#include "error.h"
#include "driver.h"
#include "drivertable.h"
*/

#include <libplayercore/playercore.h>

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzPosition : public Driver
{
  // Constructor
  public: GzPosition(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzPosition();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  public: virtual int ProcessMessage( MessageQueue *resp_queue, 
                                      player_msghdr *hdr, 
                                      void *data);
  // Handle geometry requests
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Handle motor configuration
  private: void HandleMotorPower(void *client, void *req, int reqlen);

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
Driver* GzPosition_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzPosition(cf, section)));
}


// a driver registration function
void GzPosition_Register(DriverTable* table)
{
  table->AddDriver("gz_position", GzPosition_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzPosition::GzPosition(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_POSITION2D_CODE)
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
GzPosition::~GzPosition()
{
  gz_position_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzPosition::Setup()
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
int GzPosition::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_position_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzPosition::Update()
{
  player_position2d_data_t data;
  struct timeval ts;

  gz_position_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);
  
    data.pos.px = this->iface->data->pos[0];
    data.pos.py = this->iface->data->pos[1];
    data.pos.pa = this->iface->data->rot[2] * 180 / M_PI;

    data.vel.px = this->iface->data->vel_pos[0];
    data.vel.py = this->iface->data->vel_pos[1];
    data.vel.pa = this->iface->data->vel_rot[2] * 180 / M_PI;

    data.stall = (uint8_t) this->iface->data->stall;

    this->Publish( this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_POSITION2D_DATA_STATE, 
                   (void*)&data, sizeof(data), &this->datatime );
 
  }

  gz_position_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
int GzPosition::ProcessMessage( MessageQueue *resp_queue, 
                                      player_msghdr *hdr, 
                                      void *data)
{
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 
        PLAYER_POSITION2D_CMD_VEL, this->device_addr))
  {
    player_position2d_cmd_vel_t *cmd;

    assert(hdr->size >= sizeof(player_position2d_cmd_vel_t));
    cmd = (player_position2d_cmd_vel_t*) data;

    gz_position_lock(this->iface, 1);
    this->iface->data->cmd_vel_pos[0] = cmd->vel.px;
    this->iface->data->cmd_vel_pos[1] = cmd->vel.py;
    this->iface->data->cmd_vel_rot[2] = cmd->vel.pa * M_PI / 180;
    gz_position_unlock(this->iface);
  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, 
        PLAYER_POSITION2D_REQ_GET_GEOM, this->device_addr))
  {
    player_position2d_geom_t geom;

    // TODO: get correct dimensions; there are for the P2AT

    geom.pose.px = 0;
    geom.pose.py = 0;
    geom.pose.pa = 0;
    geom.size.sw= 0.53;
    geom.size.sl = 0.38;

    this->Publish(this->device_addr, resp_queue,
        PLAYER_MSGTYPE_RESP_ACK, 
        PLAYER_POSITION2D_REQ_GET_GEOM, 
        &geom, sizeof(geom), NULL);

  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, 
        PLAYER_POSITION2D_REQ_MOTOR_POWER, this->device_addr))
  {
    player_position2d_power_config_t *power;

    assert((size_t) hdr->size >= sizeof(player_position2d_power_config_t));
    power = (player_position2d_power_config_t*) data;

    gz_position_lock(this->iface, 1);
    this->iface->data->cmd_enable_motors = power->state;
    gz_position_unlock(this->iface);

    this->Publish(this->device_addr, resp_queue,
        PLAYER_MSGTYPE_RESP_ACK, 
        PLAYER_POSITION2D_REQ_MOTOR_POWER, 
        &power, sizeof(power), NULL);
  }

  return 0;
}
