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
#include "error.h"
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

  // Get new data
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
  private: gz_fiducial_t *iface;

  // Timestamp of last data update
  private: double datatime;
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

  this->datatime = -1;
    
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

  // Add ourselves to the update list
  GzClient::AddDriver(this);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzFiducial::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_fiducial_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
void GzFiducial::Update()
{
  int i;
  gz_fiducial_fid_t *fid;
  player_fiducial_data_t data;
  struct timeval ts;
  
  gz_fiducial_lock(this->iface, 1);
  if( this->iface->data->time > this->datatime )
  {
    this->datatime = this->iface->data->time;
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod (this->iface->data->time, 1) * 1e6);

    for (i = 0; i < this->iface->data->fid_count; i++)
    {
      fid = this->iface->data->fids + i;
      if (i >= PLAYER_FIDUCIAL_MAX_SAMPLES)
        break;

      data.fiducials[i].id = htons((int16_t) fid->id);

#ifdef HAVE_GZ_FID_POSE
      // Gazebo 0.4
      //convert from range and bearing to x and y
      double r = fid->pose[0];
      double b = fid->pose[1];

      data.fiducials[i].pos[0] = htonl((int32_t) (r * cos( b ) * 1000.0));
      data.fiducials[i].pos[1] = htonl((int32_t) (r * sin( b ) * 1000.0));
      data.fiducials[i].rot[2] = htonl((int32_t) (fid->pose[2] * 1000.0));
#else
      // Gazebo 0.5
      data.fiducials[i].pos[0] = htonl((int32_t) (fid->pos[0] * 1000.0));
      data.fiducials[i].pos[1] = htonl((int32_t) (fid->pos[1] * 1000.0));
      data.fiducials[i].pos[2] = htonl((int32_t) (fid->pos[2] * 1000.0));      
      data.fiducials[i].rot[0] = htonl((int32_t) (fid->rot[0] * 1000.0));
      data.fiducials[i].rot[1] = htonl((int32_t) (fid->rot[1] * 1000.0));
      data.fiducials[i].rot[2] = htonl((int32_t) (fid->rot[2] * 1000.0));
#endif
    }
    data.count = htons(i);

    this->PutData( &data, sizeof(data), &ts );
  
  }

  gz_fiducial_unlock(this->iface);

  return;

}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzFiducial::PutCommand(player_device_id_t id,
                            void* src, size_t len,
                            struct timeval* timestamp)
{

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzFiducial::PutConfig(player_device_id_t id, void *client, 
                          void* src, size_t len,
                          struct timeval* timestamp)
{
  uint8_t subtype;

  subtype = ((uint8_t*) src)[0];
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

      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, &rep, sizeof(rep),NULL) != 0)
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
