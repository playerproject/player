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
// Desc: Gazebo (simulator) fiducial driver
// Author: Andrew Howard
// Date: 6 Apr 2003
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
#include <sys/types.h>
#include <netinet/in.h>
#include <stdlib.h>       // for atoi(3)

#include "player.h"
#include "driver.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzFiducial : public Driver
{
  // Constructor
  public: GzFiducial(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzFiducial();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Commands
  public: virtual void PutCommand(player_device_id_t id, void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t id, player_device_id_t* device, void* client, void* data, size_t len);

  // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_fiducial_t *iface;

  // Timestamp on last data update
  private: uint32_t tsec, tusec;
};


// Initialization function
Driver* GzFiducial_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzFiducial(cf, section)));
}


// a driver registration function
void GzFiducial_Register(DriverTable* table)
{
  table->AddDriver("gz_fiducial", GzFiducial_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzFiducial::GzFiducial(ConfigFile* cf, int section)
    : Driver(cf, section, PLAYER_FIDUCIAL_CODE, PLAYER_ALL_MODE,
             sizeof(player_fiducial_data_t), 0, 10, 10)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_fiducial_alloc();

  this->tsec = this->tusec = 0;
    
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzFiducial::~GzFiducial()
{
  gz_fiducial_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzFiducial::Setup()
{ 
  // Open the interface
  if (gz_fiducial_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzFiducial::Shutdown()
{
  gz_fiducial_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t GzFiducial::GetData(void* client, unsigned char* dest, size_t len,
                           uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int i;
  gz_fiducial_fid_t *fid;
  player_fiducial_data_t data;
  uint32_t tsec, tusec;
  
  gz_fiducial_lock(this->iface, 1);

  for (i = 0; i < this->iface->data->fid_count; i++)
  {
    fid = this->iface->data->fids + i;

    if (i >= PLAYER_FIDUCIAL_MAX_SAMPLES)
      break;

    data.fiducials[i].id = htons((int) fid->id);
    data.fiducials[i].pose[0] = htons((int) (fid->pose[0] * 1000.0));
    data.fiducials[i].pose[1] = htons((int) (fid->pose[1] * 180 / M_PI));
    data.fiducials[i].pose[2] = htons((int) (fid->pose[2] * 180 / M_PI));
  }
  data.count = htons(i);
  
  assert(len >= sizeof(data));
  memcpy(dest, &data, sizeof(data));

  tsec = (int) (this->iface->data->time);
  tusec = (int) (fmod(this->iface->data->time, 1) * 1e6);

  gz_fiducial_unlock(this->iface);

  // signal that new data is available
  if (tsec != this->tsec || tusec != this->tusec)
    DataAvailable();
  this->tsec = tsec;
  this->tusec = tusec;

  if (timestamp_sec)
    *timestamp_sec = tsec;
  if (timestamp_usec)
    *timestamp_usec = tusec;

  return sizeof(data);
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzFiducial::PutCommand(player_device_id_t id, void* client, unsigned char* src, size_t len)
{  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzFiducial::PutConfig(player_device_id_t id, player_device_id_t* device, void* client, void* data, size_t len)
{
  uint8_t subtype;

  subtype = ((uint8_t*) data)[0];
  switch (subtype)
  {
    case PLAYER_FIDUCIAL_GET_GEOM:
    {
      player_fiducial_geom_t rep;

      // TODO: get geometry from somewhere
      rep.subtype = PLAYER_FIDUCIAL_GET_GEOM;
      rep.pose[0] = htons((int) (0.0));
      rep.pose[1] = htons((int) (0.0));
      rep.pose[2] = htons((int) (0.0));
      rep.size[0] = htons((int) (0.0));
      rep.size[1] = htons((int) (0.0));
      rep.fiducial_size[0] = htons((int) (0.05 * 1000));
      rep.fiducial_size[1] = htons((int) (0.50 * 1000));

      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &rep, sizeof(rep)) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }

    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }
  return 0;
}
