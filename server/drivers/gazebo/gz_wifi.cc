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
// Desc: Gazebo (simulator) wifi driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_WIFI
#warning "gz_wifi not supported by libgazebo; skipping"
#else

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
class GzWifi : public CDevice
{
  // Constructor
  public: GzWifi(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzWifi();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);


  // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_wifi_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
CDevice* GzWifi_Init(char* interface, ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_WIFI_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_wifi\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new GzWifi(interface, cf, section)));
}


// a driver registration function
void GzWifi_Register(DriverTable* table)
{
  table->AddDriver("gz_wifi", PLAYER_READ_MODE, GzWifi_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzWifi::GzWifi(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_wifi_data_t), 0, 10, 10)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_wifi_alloc();
  
  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzWifi::~GzWifi()
{
  gz_wifi_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzWifi::Setup()
{ 
  // Open the interface
  if (gz_wifi_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzWifi::Shutdown()
{
  gz_wifi_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzWifi::Update()
{
  player_wifi_data_t data;
  uint32_t tsec, tusec;
  
  gz_wifi_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    tsec = (int) (this->iface->data->time);
    tusec = (int) (fmod(this->iface->data->time, 1) * 1e6);
    data.link_count= this->iface->data->link_count;
    // data.bitrate=this->iface->data->bitrate;
    //   printf("Link COunt : %d\n",data.link_count);
    for(int i=0;i<data.link_count;i++)
      {      
	data.links[i].qual=htons(0x0001);
	memcpy(data.links[i].ip,this->iface->data->links[i].ip,32);

	data.links[i].level=(uint16_t)htons((uint16_t)(this->iface->data->links[i].level));

      }
    // printf("\n");
    
    this->PutData(&data, sizeof(data), tsec, tusec);
  }

  gz_wifi_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzWifi::PutCommand(void* client, unsigned char* src, size_t len)
{  
  return;
}



#endif
