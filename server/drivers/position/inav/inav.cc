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
// Desc: "Incremental" navigation driver.
// Author: Andrew Howard
// Date: 20 Aug 2002
// CVS: $Id$
//
// Theory of operation:
//  The inav driver uses incremental mapping to build a local occupancy
//  grid and estimate the robot's pose with respect to this grid.  The pose
//  estimates are generally better than those produced using odometry alone,
//  particularly when the robot is turning in place.
//
//  The inav driver also implements a position controller with built in
//  obstacle avoidance.
//
// Requires: position (odometry), laser
// Provides: position
//
///////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class INav : public CDevice
{
  // Constructor
  public: INav(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Set up the underlying position device.
  private: int SetupPosition();
  private: int ShutdownPosition();

  // Set up the underlying laser device.
  private: int SetupLaser();
  private: int ShutdownLaser();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Get the current odometric pose
  private: void ReadOdoPose(double pose[3]);

  // Get the current laser scan
  private: void ReadLaserRanges();

  // Geometry of underlying position device.
  private: player_position_geom_t geom;

  // Position device info (the one we are subscribed to).
  private: int position_index;
  private: CDevice *position;

  // Laser device info
  private: int laser_index;
  private: CDevice *laser;

  // Laser pose in robot cs
  private: double laser_pose[3];

  // Current odometric pose estimate
  private: double odo_pose[3];
  
  // Current incremental pose estimate
  private: double inc_pose[3];
};


// Initialization function
CDevice* INav_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_POSITION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"inav\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new INav(interface, cf, section)));
}


// a driver registration function
void INav_Register(DriverTable* table)
{
  table->AddDriver("inav", PLAYER_ALL_MODE, INav_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
INav::INav(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), sizeof(player_position_cmd_t), 10, 10)
{
  this->position_index = cf->ReadInt(section, "position_index", 0);
  this->position = NULL;

  this->laser_index = cf->ReadInt(section, "laser_index", 0);
  this->laser = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int INav::Setup()
{
  // Initialise the underlying position device.
  if (this->SetupPosition() != 0)
    return -1;

  // Initialise the laser.
  if (this->SetupLaser() != 0)
    return -1;

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int INav::Shutdown()
{
  // Stop the driver thread.
  this->StopThread();

  // Stop the laser
  this->ShutdownLaser();

  // Stop the position device.
  this->ShutdownPosition();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying position device.
int INav::SetupPosition()
{
  player_device_id_t id;

  id.code = PLAYER_POSITION_CODE;
  id.index = this->position_index;
  id.port = this->device_id.port;

  this->position = deviceTable->GetDevice(id);
  if (!this->position)
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return -1;
  }
  
  if (this->position->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return -1;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying position device.
int INav::ShutdownPosition()
{
  this->position->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int INav::SetupLaser()
{
  player_device_id_t id;

  id.code = PLAYER_LASER_CODE;
  id.index = this->laser_index;
  id.port = this->device_id.port;

  this->laser = deviceTable->GetDevice(id);
  if (!this->laser)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return -1;
  }
  if (this->laser->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return -1;
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int INav::ShutdownLaser()
{
  this->laser->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void INav::Main() 
{
  struct timespec sleeptime;

  // Sleep for 1ms (will actually take longer than this).
  sleeptime.tv_sec = 0;
  sleeptime.tv_nsec = 1000000L;
  
  while (true)
  {
    // Go to sleep for a while (this is a polling loop).
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    HandleRequests();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int INav::HandleRequests()
{
  int len;
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      /* TODO
      case PLAYER_POSITION_GET_GEOM_REQ:
        HandleGetGeom(client, request, len);
        break;
      */

      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle geometry requests.
void INav::HandleGetGeom(void *client, void *request, int len)
{
  /* TODO
  player_device_id_t id;
  uint8_t req;
  player_position_geom_t geom;
  struct timeval ts;
  uint16_t reptype;

  if (len != 1)
  {
    PLAYER_ERROR2("geometry request len is invalid (%d != %d)", len, 1);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  id.code = PLAYER_POSITION_CODE;
  id.index = this->position_index;
  id.port = this->device_id.port;
  
  // Get underlying device geometry.
  req = PLAYER_POSITION_GET_GEOM_REQ;
  if (this->Request(&id, this, &req, 1, &reptype, &ts, &geom, sizeof(geom)) != 0)
  {
    PLAYER_ERROR("unable to get position device geometry");
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }
  if (reptype != PLAYER_MSGTYPE_RESP_ACK)
  {
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");
  */
  
  return;
}



////////////////////////////////////////////////////////////////////////////////
// Update the position device (returns non-zero if changed).
int INav::UpdatePosition()
{
  int i;
  size_t size;
  player_position_data_t data;
  uint32_t timesec, timeusec;
  double time;
  
  // Get the position device data.
  size = this->position->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->position_time < 0.001)
    return 0;
  this->position_time = time;

  // Byte swap
  data.xpos = ntohl(data.xpos);
  data.ypos = ntohl(data.ypos);
  data.yaw = ntohl(data.yaw);

  this->position_new_pose[0] = (double) data.xpos / 1000.0;
  this->position_new_pose[1] = (double) data.ypos / 1000.0;
  this->position_new_pose[2] = (double) data.yaw * M_PI / 180;
  
  return 1;
}



////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void INav::UpdateData()
{
  uint32_t timesec, timeusec;
  player_position_data_t data;

  data.xpos = (int32_t) (this->pose[0] * 1000);
  data.ypos = (int32_t) (this->pose[1] * 1000);
  data.yaw = (int32_t) (this->pose[2] * 180 / M_PI);

  // Byte swap
  data.xpos = htonl(data.xpos);
  data.ypos = htonl(data.ypos);
  data.yaw = htonl(data.yaw);

  // Compute time.  Use the position device's time.
  timesec = (uint32_t) this->position_time;
  timeusec = (uint32_t) (fmod(this->position_time, 1.0) * 1e6);

  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);

  return;
}
