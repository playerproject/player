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
// Desc: Gazebo (simulator) pan tilt zoom driver
// Author: Pranav Srivastava
// Date: 16 Sep 2003
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
#include "device.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzPtz : public CDevice
{
  // Constructor
  public: GzPtz(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzPtz();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  // public: virtual int PutConfig(player_device_id_t* device, void* client,
  //                                void* req, size_t reqlen);

  // Handle geometry requests.
// private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Gazebo id
  private: const char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_ptz_t *iface;
};


// Initialization function
CDevice* GzPtz_Init(char* interface, ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_PTZ_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_ptz\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzPtz(interface, cf, section)));
}


// a driver registration function
void GzPtz_Register(DriverTable* table)
{
  table->AddDriver("gz_ptz", PLAYER_ALL_MODE, GzPtz_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzPtz::GzPtz(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_ptz_data_t), sizeof(player_ptz_cmd_t), 10, 10)
{

  // Get the id of the device in Gazebo
  this->gz_id = cf->ReadString(section, "gz_id", 0);

  // Get the globally defined Gazebo client (one per instance of Player)
  this->client = GzClient::client;
  
  // Create an interface
  this->iface = gz_ptz_alloc();

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzPtz::~GzPtz()
{
  gz_ptz_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzPtz::Setup()
{
  // Open the interface
  if (gz_ptz_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzPtz::Shutdown()
{
  gz_ptz_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t GzPtz::GetData(void* client, unsigned char* dest, size_t len,
                           uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
   player_ptz_data_t data;

   data.pan = htonl((int) (this->iface->data->pan * 180/M_PI));
  data.tilt = htonl((int) (this->iface->data->tilt * 180 /M_PI));
  data.zoom = htonl((int) (this->iface->data->zoom * 180 / M_PI));
  
  assert(len >= sizeof(data));
  memcpy(dest, &data, sizeof(data));

  if (timestamp_sec)
    *timestamp_sec = (int) (this->iface->data->time);
  if (timestamp_usec)
    *timestamp_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);
  
  return sizeof(data);
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzPtz::PutCommand(void* client, unsigned char* src, size_t len)
{
  player_ptz_cmd_t *cmd;

  assert(len >= sizeof(player_ptz_cmd_t));
  cmd = (player_ptz_cmd_t*) src;

  this->iface->data->cmd_pan = ((int) ntohl(cmd->pan)) * M_PI/180;
  this->iface->data->cmd_tilt = ((int) ntohl(cmd->tilt)) * M_PI/180;
  this->iface->data->cmd_zoom = ((int) ntohl(cmd->zoom)) * M_PI / 180;
  
  return;
}

/*
////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzPtz::PutConfig(player_device_id_t* device, void* client, void* req, size_t req_len)
{
  switch (((char*) req)[0])
  {
    case PLAYER_POSITION_GET_GEOM_REQ:
      HandleGetGeom(client, req, req_len);
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
void GzPtz::HandleGetGeom(void *client, void *req, int reqlen)
{
  // player_position_geom_t geom;

  // TODO: get correct dimensions; there are for the P2AT
  
  geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
  geom.pose[0] = htons((int) (0));
  geom.pose[1] = htons((int) (0));
  geom.pose[2] = htons((int) (0));
  geom.size[0] = htons((int) (0.53 * 1000));
  geom.size[1] = htons((int) (0.38 * 1000));

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}
*/
