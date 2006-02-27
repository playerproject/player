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
// Desc: Gazebo (simulator) sonar driver
// Author: Carle Cote (Laborius - Universite de Sherbrooke - Sherbrooke, Quebec, Canada)
// Date: 23 february 2004
//
// Modification
// By:                     Description                            Date
// Victor Tran             implementation to take information     March 23, 2004
//                         and the data in Update() and
//                         HandleGetGeom()
//
// CVS:
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup player_driver_gz_sonar gz_sonar
 * @brief Gazebo sonar

@todo This driver is currently disabled because it needs to be updated to
the Player 2.0 API.

*/
/** @} */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef INCLUDE_GAZEBO_SONAR
#warning "gz_sonar not supported by libgazebo; skipping"
#else

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


// Gazebo Sonar driver
class GzSonar : public Driver
{
  // Constructor
  public: GzSonar(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzSonar();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();
  public: int ProcessMessage( MessageQueue *resp_queue, 
                              player_msghdr *hdr, 
                              void *data);

  // Handle geometry requests
  private: void HandleGetGeom( MessageQueue *resp_queue, 
                               player_msghdr *hdr, 
                               void *data);

  // Handle sonar configuration
  private: void HandleSonarPower( MessageQueue *resp_queue, 
                                  player_msghdr *hdr, 
                                  void *data);

  // Gazebo id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_sonar_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzSonar_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzSonar(cf, section)));
}


// a driver registration function
void GzSonar_Register(DriverTable* table)
{
  table->AddDriver("gz_sonar", GzSonar_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzSonar::GzSonar(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_SONAR_CODE)
{
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));
  
  // Create an interface
  this->iface = gz_sonar_alloc();

  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzSonar::~GzSonar()
{
  gz_sonar_free(this->iface); 
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzSonar::Setup()
{
  // Open the interface
  if (gz_sonar_open(this->iface, this->client, this->gz_id) != 0)
    return -1;
  
  // Add ourselves to the update list
  GzClient::AddDriver(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzSonar::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_sonar_close(this->iface);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzSonar::Update()
{
  player_sonar_data_t data;
  struct timeval ts;

  gz_sonar_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;

    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    data.ranges_count = this->iface->data->sonar_count;
    for (int i = 0; i < this->iface->data->sonar_count; i++)
      data.ranges[i] = this->iface->data->sonar_ranges[i];

    this->Publish( this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_SONAR_DATA_RANGES,     
                   (void*)&data, sizeof(data), &this->datatime );
  }

  gz_sonar_unlock(this->iface);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process Messages
int GzSonar::ProcessMessage( MessageQueue *resp_queue, 
                             player_msghdr *hdr, 
                             void *data)
{
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SONAR_REQ_GET_GEOM, this->device_addr))
  {
    this->HandleGetGeom(resp_queue, hdr, data);
  }
  else if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_SONAR_REQ_POWER,this->device_addr))
  {
    this->HandleSonarPower(resp_queue, hdr, data);
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void GzSonar::HandleGetGeom( MessageQueue *resp_queue, 
                             player_msghdr *hdr, 
                             void *data)
{
  player_sonar_geom_t geom;

  gz_sonar_lock(this->iface, 1);

  geom.poses_count = this->iface->data->sonar_count;

  //the position of valid sonar
  for (int i = 0; i < this->iface->data->sonar_count; i++)
  {
    geom.poses[i].px = this->iface->data->sonar_pos[i][0];
    geom.poses[i].py = this->iface->data->sonar_pos[i][1];
    geom.poses[i].pa = this->iface->data->sonar_rot[i][2] * 180 / M_PI;
  }
  gz_sonar_unlock(this->iface);

  this->Publish(this->device_addr, resp_queue,
                PLAYER_MSGTYPE_RESP_ACK, 
                PLAYER_SONAR_REQ_GET_GEOM,
                &geom, sizeof(geom), NULL);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle sonar power 
void GzSonar::HandleSonarPower( MessageQueue *resp_queue, 
                             player_msghdr *hdr, 
                             void *data)
{
  player_sonar_power_config_t *power;
  
  assert((size_t) hdr->size >= sizeof(player_sonar_power_config_t));
  power = (player_sonar_power_config_t*) data;

  gz_sonar_lock(this->iface, 1);

  // TODO
  
  gz_sonar_unlock(this->iface);

   this->Publish(this->device_addr, resp_queue,
                PLAYER_MSGTYPE_RESP_ACK, 
                PLAYER_SONAR_REQ_POWER,
                power, sizeof(*power), NULL);
 
  return;
}

#endif
