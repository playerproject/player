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
// Desc: Gazebo (simulator) HUD (heads up display) driver
// Author: Nate Koenig 
// Date: 8 Jun 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_HUD
#warning "gz_hud not supported by libgazebo; skipping"
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
class GzHUD : public CDevice
{
  // Constructor
  public: GzHUD(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzHUD();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  // Commands
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);

  // Request/reply
  public: virtual int PutConfig(player_device_id_t* device, void* client, void* data, size_t len);

  // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_hud_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
CDevice* GzHUD_Init(char* interface, ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  if (strcmp(interface, PLAYER_SIMULATION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_hud\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }

  return ((CDevice*) (new GzHUD(interface, cf, section)));
}


// a driver registration function
void GzHUD_Register(DriverTable* table)
{
  printf ("Registering HUD\n");
  table->AddDriver("gz_hud", PLAYER_ALL_MODE, GzHUD_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzHUD::GzHUD(char* interface, ConfigFile* cf, int section)
    : CDevice(0, 0, 10, 10)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_hud_alloc();
  
  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzHUD::~GzHUD()
{
  gz_hud_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzHUD::Setup()
{ 

  // Open the interface
  if (gz_hud_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzHUD::Shutdown()
{
  gz_hud_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzHUD::Update()
{
  // The HUD doesn't return data
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Commands
void GzHUD::PutCommand(void* client, unsigned char* src, size_t len)
{  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzHUD::PutConfig(player_device_id_t* device, void* client, void* data, size_t len)
{
  player_hud_config_t *cfg = (player_hud_config_t*)(data);
  gz_hud_draw_t *hud;

  gz_hud_lock(this->iface,1);

  hud = &(this->iface->data->queue[this->iface->data->index]);
  this->iface->data->index++;

  // Id of this element
  hud->id = ((int32_t)ntohl(cfg->id));

  // Should this element be removed?
  hud->remove = (int8_t)cfg->remove;


  switch (cfg->subtype)
  {
    case PLAYER_HUD_BOX:
    {
      hud->type = GAZEBO_HUD_BOX;
      break;
    }

    case PLAYER_HUD_LINE:
    {
      hud->type = GAZEBO_HUD_LINE;
      break;
    }

    case PLAYER_HUD_TEXT:
    {
      hud->type = GAZEBO_HUD_TEXT;
      break;
    }

    case PLAYER_HUD_CIRCLE:
    {
      hud->type = GAZEBO_HUD_CIRCLE;
      break;
    }

    default:
    {
      if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
  }

  if (!hud->remove)
  {
    // Set point 1
    hud->pt1[0] = ((int16_t)ntohs(cfg->pt1[0]));
    hud->pt1[1] = ((int16_t)ntohs(cfg->pt1[1]));

    // Set point 2
    hud->pt2[0] = ((int16_t)ntohs(cfg->pt2[0]));
    hud->pt2[1] = ((int16_t)ntohs(cfg->pt2[1]));

    // Set values
    hud->value1 = ((int16_t)ntohs(cfg->value1));

    // Set text
    strncpy(hud->text, cfg->text, PLAYER_MAX_DEVICE_STRING_LEN);

    // Set the color
    hud->color[0] = ((int16_t)ntohs(cfg->color[0])) / 100.0;
    hud->color[1] = ((int16_t)ntohs(cfg->color[1])) / 100.0;
    hud->color[2] = ((int16_t)ntohs(cfg->color[2])) / 100.0;

    hud->filled = (int8_t)cfg->filled;
  }


  gz_hud_unlock(this->iface);

  PutReply(client, PLAYER_MSGTYPE_RESP_NACK);

  return 0;
}

#endif
