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
// Desc: Adaptive Monte-Carlo localization
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
// Theory of operation:
//  TODO
//
// Requires: position (odometry), laser
// Provides: localization
//
///////////////////////////////////////////////////////////////////////////

#include <assert.h>
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


// Incremental navigation driver
class AdaptiveMCL : public CDevice
{
  // Constructor
  public: AdaptiveMCL(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~AdaptiveMCL();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();
  
  // Set up the odometry device.
  private: int SetupOdom();
  private: int ShutdownOdom();

  // Set up the laser device.
  private: int SetupLaser();
  private: int ShutdownLaser();

  // Main function for device thread.
  private: virtual void Main();

  // Check for new odometry data
  private: int GetOdom();

  // Check for new laser data
  private: int GetLaser();

  // Update the filter with new sensor data
  private: int UpdateFilter();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Odometry device info
  private: CDevice *odom;
  private: int odom_index;
  private: double odom_time;
  
  // Laser device info
  private: CDevice *laser;
  private: int laser_index;
  private: double laser_time;

  // Laser range and bearing values
  private: int laser_count;
  private: double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];
};


// Initialization function
CDevice* AdaptiveMCL_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_LOCALIZATION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"adaptive_mcl\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new AdaptiveMCL(interface, cf, section)));
}


// a driver registration function
void AdaptiveMCL_Register(DriverTable* table)
{
  table->AddDriver("adaptive_mcl", PLAYER_ALL_MODE, AdaptiveMCL_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
AdaptiveMCL::AdaptiveMCL(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_position_data_t), sizeof(player_position_cmd_t), 10, 10)
{
  double size;
  
  this->odom = NULL;
  this->odom_index = cf->ReadInt(section, "position_index", 0);
  this->odom_time = 0.0;

  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", 0);
  this->laser_time = 0.0;

  // TESTING; should get this by querying the laser geometry
  /*
  this->laser_pose.v[0] = cf->ReadTupleLength(section, "laser_pose", 0, 0);
  this->laser_pose.v[1] = cf->ReadTupleLength(section, "laser_pose", 1, 0);
  this->laser_pose.v[2] = cf->ReadTupleAngle(section, "laser_pose", 2, 0);
  */

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
AdaptiveMCL::~AdaptiveMCL()
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int AdaptiveMCL::Setup()
{
  // Initialise the underlying position device.
  if (this->SetupOdom() != 0)
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
int AdaptiveMCL::Shutdown()
{
  // Stop the driver thread.
  this->StopThread();

  // Stop the laser
  this->ShutdownLaser();

  // Stop the odom device.
  this->ShutdownOdom();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int AdaptiveMCL::SetupOdom()
{
  player_device_id_t id;

  id.robot = this->device_id.robot;
  id.code = PLAYER_POSITION_CODE;
  id.index = this->odom_index;

  this->odom = deviceTable->GetDevice(id);
  if (!this->odom)
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return -1;
  }
  
  if (this->odom->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return -1;
  }

  // TODO
  // Get the odometry geometry

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying odom device.
int AdaptiveMCL::ShutdownOdom()
{
  this->odom->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int AdaptiveMCL::SetupLaser()
{
  player_device_id_t id;
  
  id.robot = this->device_id.robot;
  id.code = PLAYER_LASER_CODE;
  id.index = this->laser_index;

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

  // TODO
  // Get the laser geometry
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int AdaptiveMCL::ShutdownLaser()
{
  this->laser->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void AdaptiveMCL::Main() 
{
  struct timespec sleeptime;

  while (true)
  {
    // Sleep for 1ms (will actually take longer than this).
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000L;
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    this->HandleRequests();
    
    // Get current odometry values
    this->GetOdom();

    // Get current laser values; if we have a new set, update the
    // filter
    if (this->GetLaser())
      this->UpdateFilter();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new odometry data
int AdaptiveMCL::GetOdom()
{
  int i;
  size_t size;
  player_position_data_t data;
  uint32_t timesec, timeusec;
  double time;
  
  // Get the odom device data.
  size = this->odom->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->odom_time < 0.001)
    return 0;
  this->odom_time = time;

  // Byte swap
  data.xpos = ntohl(data.xpos);
  data.ypos = ntohl(data.ypos);
  data.yaw = ntohl(data.yaw);

  /* TODO
  this->odom_pose.v[0] = (double) ((int32_t) data.xpos) / 1000.0;
  this->odom_pose.v[1] = (double) ((int32_t) data.ypos) / 1000.0;
  this->odom_pose.v[2] = (double) ((int32_t) data.yaw) * M_PI / 180;
  */
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new laser data
int AdaptiveMCL::GetLaser()
{
  int i;
  size_t size;
  player_laser_data_t data;
  uint32_t timesec, timeusec;
  double time;
  double r, b, db;
  
  // Get the laser device data.
  size = this->laser->GetData(this,(unsigned char*) &data, sizeof(data), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->laser_time < 0.001)
    return 0;
  this->laser_time = time;

  b = ((int16_t) ntohs(data.min_angle)) / 100.0 * M_PI / 180.0;
  db = ((int16_t) ntohs(data.resolution)) / 100.0 * M_PI / 180.0;
  this->laser_count = ntohs(data.range_count);
  assert(this->laser_count < sizeof(this->laser_ranges) / sizeof(this->laser_ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < this->laser_count; i++)
  {
    r = ((int16_t) ntohs(data.ranges[i])) / 1000.0;
    this->laser_ranges[i][0] = r;
    this->laser_ranges[i][1] = b;
    b += db;
  }
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Update the filter with new sensor data
int AdaptiveMCL::UpdateFilter()
{
  return 0;
}



////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int AdaptiveMCL::HandleRequests()
{
  int len;
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      /*
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
void AdaptiveMCL::HandleGetGeom(void *client, void *request, int len)
{
  player_device_id_t id;
  player_position_geom_t geom;
  uint8_t req;
  struct timeval ts;
  uint16_t reptype;

  printf("get geom\n");
  
  if (len != 1)
  {
    PLAYER_ERROR2("geometry request len is invalid (%d != %d)", len, 1);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  id.robot = this->device_id.robot;
  id.code = PLAYER_POSITION_CODE;
  id.index = this->odom_index;
  
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

  printf("got geom\n");
  
  return;
}
