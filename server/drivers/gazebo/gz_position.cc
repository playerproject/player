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

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "player.h"
#include "device.h"
#include "drivertable.h"

// UNBELIEVABLE HACK
#include "/home/ahoward/darpa-demo/gazebo-exp/src/gazebo.h"

// Incremental navigation driver
class GzPosition : public CDevice
{
  // Constructor
  public: GzPosition(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzPosition();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t* device, void* client,
                                void* req, size_t reqlen);

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Gazebo id
  private: const char *gz_id;

  // MMap file
  private: int mmap_fd;
  private: gz_position_t *mmap;
};


// Initialization function
CDevice* GzPosition_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_position\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzPosition(interface, cf, section)));
}


// a driver registration function
void GzPosition_Register(DriverTable* table)
{
  table->AddDriver("gz_position", PLAYER_ALL_MODE, GzPosition_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzPosition::GzPosition(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), sizeof(player_position_cmd_t), 10, 10)
{

  // Get the id of the device in Gazebo
  this->gz_id = cf->ReadString(section, "gz_id", 0);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzPosition::~GzPosition()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzPosition::Setup()
{
  char filename[128];

  snprintf(filename, sizeof(filename), "/tmp/gazebo.%s.mmap", this->gz_id);
  printf("opening %s\n", filename);

  // Open the mmap file
  this->mmap_fd = ::open(filename, O_RDWR);
  if (this->mmap_fd <= 0)
  {
    PLAYER_ERROR1("error opening device file: %s", strerror(errno));
    return -1;
  }

  // Map the mmap file
  this->mmap = (gz_position_t*) ::mmap(0, sizeof(gz_position_t), PROT_READ | PROT_WRITE,
                                       MAP_SHARED, this->mmap_fd, 0);
  if ((int) this->mmap <= 0)
  {
    PLAYER_ERROR1("error mapping device file: %s", strerror(errno));
    exit(1);
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzPosition::Shutdown()
{
  ::munmap(this->mmap, sizeof(gz_position_t));
  ::close(this->mmap_fd);

  this->mmap = NULL;
  this->mmap_fd = 0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t GzPosition::GetData(void* client, unsigned char* dest, size_t len,
                           uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  player_position_data_t data;

  data.xpos = htonl((int) (this->mmap->odom_pose[0] * 1000));
  data.ypos = htonl((int) (this->mmap->odom_pose[1] * 1000));
  data.yaw = htonl((int) (this->mmap->odom_pose[2] * 180 / M_PI));
  
  assert(len >= sizeof(data));
  memcpy(dest, &data, sizeof(data));

  if (timestamp_sec)
    *timestamp_sec = (int) (this->mmap->time);
  if (timestamp_usec)
    *timestamp_usec = (int) (fmod(this->mmap->time, 1) * 1e6);
  
  return sizeof(data);
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzPosition::PutCommand(void* client, unsigned char* src, size_t len)
{
  player_position_cmd_t *cmd;

  assert(len >= sizeof(player_position_cmd_t));
  cmd = (player_position_cmd_t*) src;

  this->mmap->cmd_vel[0] = ((int) ntohl(cmd->xspeed)) / 1000.0;
  this->mmap->cmd_vel[1] = ((int) ntohl(cmd->yspeed)) / 1000.0;
  this->mmap->cmd_vel[2] = ((int) ntohl(cmd->yawspeed)) * M_PI / 180;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzPosition::PutConfig(player_device_id_t* device, void* client, void* req, size_t req_len)
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
void GzPosition::HandleGetGeom(void *client, void *req, int reqlen)
{
  player_position_geom_t geom;

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
