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
// Desc: Gazebo (simulator) factory driver
// Author: Chris Jones
// Date: 14 Nov 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_FACTORY
#warning "gz_factory not supported by libgazebo; skipping"
#else

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
class GzFactory : public CDevice
{
  // Constructor
  public: GzFactory(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzFactory();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t* device, void* client,
                                void* req, size_t reqlen);

  // Gazebo id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_factory_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
CDevice* GzFactory_Init(char* interface, ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_SPEECH_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_factory\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzFactory(interface, cf, section)));
}


// a driver registration function
void GzFactory_Register(DriverTable* table)
{
  table->AddDriver("gz_factory", PLAYER_ALL_MODE, GzFactory_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzFactory::GzFactory(char* interface, ConfigFile* cf, int section)
  : CDevice(0, sizeof(player_speech_cmd_t), 10, 10)
{
    // Get the globally defined Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Create an interface
  this->iface = gz_factory_alloc();

  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzFactory::~GzFactory()
{
  gz_factory_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzFactory::Setup()
{
  // Open the interface
  if (gz_factory_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzFactory::Shutdown()
{
  gz_factory_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzFactory::Update()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzFactory::PutCommand(void* client, unsigned char* src, size_t len)
{
  player_speech_cmd_t *cmd;
    
  assert(len >= sizeof(player_speech_cmd_t));
  cmd = (player_speech_cmd_t*) src;

  gz_factory_lock(this->iface, 1);
  strcpy((char *)this->iface->data->string, (char *)cmd->string);
  gz_factory_unlock(this->iface);
    
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzFactory::PutConfig(player_device_id_t* device, void* client, void* req, size_t req_len)
{
  switch (((char*) req)[0])
  {
    default:
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
  }
  return 0;
}

#endif
