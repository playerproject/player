#ifndef GAZEBO_CAMERA
#define GAZEBO_CAMERA
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
// Desc: Gazebo (simulator) camera driver
// Author: Pranav Srivastava
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
#include "device.h"
#include "drivertable.h"

#include "gazebo.h"
#include "gz_client.h"


// Incremental navigation driver
class GzCamera : public CDevice
{
  // Constructor
  public: GzCamera(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzCamera();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Data
  public: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // Gazebo device id
  private: const char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  public: gz_camera_t *iface;
};


// Initialization function
CDevice* GzCamera_Init(char* interface, ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_CAMERA_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_camera\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzCamera(interface, cf, section)));
}


// a driver registration function
void GzCamera_Register(DriverTable* table)
{
  table->AddDriver("gz_camera", PLAYER_ALL_MODE, GzCamera_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzCamera::GzCamera(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_camera_data_t), 0, 10, 10)
{

  // Get the id of the device in Gazebo
  this->gz_id = cf->ReadString(section, "gz_id", 0);

  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;
  
  // Create an interface
  this->iface = gz_camera_alloc();
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzCamera::~GzCamera()
{
  gz_camera_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzCamera::Setup()
{ 
  // Open the interface
  if (gz_camera_open(this->iface, this->client, this->gz_id,sizeof(gz_camera_data_t)) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzCamera::Shutdown()
{
  gz_camera_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Data
size_t GzCamera::GetData(void* client, unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  player_camera_data_t data;

  data.image =(unsigned char*) ((this->iface->data->image));
  
  assert(len >= sizeof(data));
  memcpy(dest, &data, sizeof(data));

  if (timestamp_sec)
    *timestamp_sec = (int) (this->iface->data->time);
  if (timestamp_usec)
    *timestamp_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);
   
  return sizeof(data);
}


#endif


































