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

  public: virtual int ProcessMessage( MessageQueue *resp_queue, 
                                      player_msghdr *hdr, 
                                      void *data);
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
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_TRUTH_CODE)
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

  // Add ourselves to the update list
  GzClient::AddDriver(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzTruth::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_truth_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzTruth::Update()
{
  player_truth_pose_t data;
  double e[3];
  struct timeval ts;
  
  gz_truth_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    data.pose.px = this->iface->data->pos[0];
    data.pose.py = this->iface->data->pos[1];
    data.pose.pz = this->iface->data->pos[2];

    // Convert the rotation from quaternion to euler 
    gz_truth_euler_from_quatern(this->iface, e, this->iface->data->rot);

    data.pose.proll = e[0];
    data.pose.ppitch = e[1];
    data.pose.pyaw = e[2];
    
    this->Publish( this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_TRUTH_DATA_POSE, 
                   (void*)&data, sizeof(data), &this->datatime );
 
  }

  gz_truth_unlock(this->iface);

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzTruth::ProcessMessage( MessageQueue *resp_queue, 
                             player_msghdr *hdr, 
                             void *data)
{
  double q[4];
  double e[3];

  switch (hdr->subtype)
  {
    case PLAYER_TRUTH_REQ_SET_POSE:
    {
      player_truth_pose_t *req = (player_truth_pose_t*) data;
      
      gz_truth_lock(this->iface, 1);

      this->iface->data->cmd_pos[0] = req->pose.px;
      this->iface->data->cmd_pos[1] = req->pose.py;
      this->iface->data->cmd_pos[2] = req->pose.pz;

      e[0] = req->pose.proll;
      e[1] = req->pose.ppitch;
      e[2] = req->pose.pyaw;

      gz_truth_quatern_from_euler(this->iface, q,e);

      this->iface->data->cmd_rot[0] = q[0];
      this->iface->data->cmd_rot[1] = q[1];
      this->iface->data->cmd_rot[2] = q[2];
      this->iface->data->cmd_rot[3] = q[3];

      this->iface->data->cmd_new = 1;

      gz_truth_unlock(this->iface);

      this->Publish(this->device_addr, resp_queue, 
                    PLAYER_MSGTYPE_RESP_ACK,
                    PLAYER_TRUTH_REQ_SET_POSE,
                    &req, sizeof(req), NULL);
      break;
    }
    default:
    {
      return (PLAYER_MSGTYPE_RESP_NACK);
    }
  }

  return 0;
}

#endif
