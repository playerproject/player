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
// Desc: Gazebo (simulator) laser driver
// Author: Andrew Howard
// Date: 6 Apr 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_gz_laser gz_laser


The gz_laser driver is used to access Gazebo models that support the
libgazebo laser interface (such as the SickLMS200 model).

@par Compile-time dependencies

- Gazebo

@par Provides

- @ref player_interface_laser

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- gz_id (string)
  - Default: ""
  - ID of the Gazebo model.
      
@par Example 

@verbatim
driver
(
  name gz_laser
  provides ["laser:0"]
  gz_id "laser1"
)
@endverbatim

@par Authors

Andrew Howard
*/
/** @} */

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
class GzLaser : public Driver
{
  // Constructor
  public: GzLaser(ConfigFile* cf, int section);

  // Destructor
  public: virtual ~GzLaser();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Check for new data
  public: virtual void Update();

  public: int ProcessMessage(MessageQueue *resp_queue, player_msghdr *hdr, void *data);

   // Gazebo device id
  private: char *gz_id;

  // Gazebo client object
  private: gz_client_t *client;
  
  // Gazebo Interface
  private: gz_laser_t *iface;

  // Timestamp on last data update
  private: double datatime;
};


// Initialization function
Driver* GzLaser_Init(ConfigFile* cf, int section)
{
  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  return ((Driver*) (new GzLaser(cf, section)));
}


// a driver registration function
void GzLaser_Register(DriverTable* table)
{
  table->AddDriver("gz_laser", GzLaser_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
GzLaser::GzLaser(ConfigFile* cf, int section)
    : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_LASER_CODE)
{
  // Get the globally defined  Gazebo client (one per instance of Player)
  this->client = GzClient::client;

  // Get the id of the device in Gazebo.
  // TODO: fix potential buffer overflow
  this->gz_id = (char*) calloc(1024, sizeof(char));
  strcat(this->gz_id, GzClient::prefix_id);
  strcat(this->gz_id, cf->ReadString(section, "gz_id", ""));

  // Create an interface
  this->iface = gz_laser_alloc();
  
  this->datatime = -1;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
GzLaser::~GzLaser()
{
  gz_laser_free(this->iface);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int GzLaser::Setup()
{ 
  // Open the interface
  if (gz_laser_open(this->iface, this->client, this->gz_id) != 0)
    return -1;

  // Add ourselves to the update list
  GzClient::AddDriver(this);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int GzLaser::Shutdown()
{
  // Remove ourselves to the update list
  GzClient::DelDriver(this);

  gz_laser_close(this->iface);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new data
void GzLaser::Update()
{
  int i;
  player_laser_data_t data;
  struct timeval ts;
  double range_res, angle_res;

  gz_laser_lock(this->iface, 1);

  if (this->iface->data->time > this->datatime)
  {
    this->datatime = this->iface->data->time;
    ts.tv_sec = (int) (this->iface->data->time);
    ts.tv_usec = (int) (fmod(this->iface->data->time, 1) * 1e6);

    // Pick the rage resolution to use (1, 10, 100)
    if (this->iface->data->max_range <= 8.192)
      range_res = 1.0;
    else if (this->iface->data->max_range <= 81.92)
      range_res = 10.0;
    else
      range_res = 100.0;

    angle_res = this->iface->data->res_angle;

    //printf("range res = %f %f\n", range_res, this->iface->data->max_range);
  
    data.min_angle = this->iface->data->min_angle * 180 / M_PI;
    data.max_angle = this->iface->data->max_angle * 180 / M_PI;
    data.resolution = angle_res * 180 / M_PI;
    data.ranges_count = this->iface->data->range_count; 

    for (i = 0; i < this->iface->data->range_count; i++)
    {
      data.ranges[i] = this->iface->data->ranges[i] / range_res;
      data.intensity[i] = (uint8_t) (int) this->iface->data->intensity[i];
    }

    this->Publish( this->device_addr, NULL,
                   PLAYER_MSGTYPE_DATA,
                   PLAYER_LASER_DATA_SCAN,        
                   (void*)&data, sizeof(data), &this->datatime );
 
  }

  gz_laser_unlock(this->iface);

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Handle requests
int GzLaser::ProcessMessage(MessageQueue *resp_queue, player_msghdr *hdr, void *data)
{
  switch (hdr->subtype)
  {
    case PLAYER_LASER_REQ_GET_GEOM:
    {
      player_laser_geom_t rep;

      // TODO: get geometry from somewhere
      rep.pose.px = 0.0;
      rep.pose.py = 0.0;
      rep.pose.pa = 0.0;
      rep.size.sw = 0.0;
      rep.size.sl = 0.0;

      this->Publish(this->device_addr, resp_queue,
                    PLAYER_MSGTYPE_RESP_ACK, 
                    PLAYER_LASER_REQ_GET_GEOM, 
                    &rep, sizeof(rep),NULL);
      break;
    }

    default:
    {
      return (PLAYER_MSGTYPE_RESP_NACK);
    }
  }

  return 0;
}
