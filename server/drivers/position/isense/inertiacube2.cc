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
// Desc: Driver ISense InertiaCube2 orientation sensor.
// Author: Andrew Howard
// Date: 20 Aug 2002
// CVS: $Id$
//
// Theory of operation:
//  Uses an inertial orientation sensor to correct the odometry coming
//  from a robot.  The assumption is that the position device we
//  subscribe to has good position information but poor orientation
//  information.
//
// Requires: position
// Provides: position
//
///////////////////////////////////////////////////////////////////////////

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include <isense/isense.h>

#include "player.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"


// Driver for detecting laser retro-reflectors.
class InertiaCube2 : public Driver
{
  // Constructor
  public: InertiaCube2( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Set up the underlying position device.
  private: int SetupPosition();
  private: int ShutdownPosition();

  // Initialize the IMU.
  private: int SetupImu();
  private: int ShutdownImu();

  // Get the tracker type.
  private: const char *ImuType(int type);

  // Get the tracker model.
  private: const char *ImuModel(int model);

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Update the InertiaCube.
  private: void UpdateImu();

  // Update the position device (returns non-zero if changed).
  private: int UpdatePosition();

  // Generate a new pose estimate.
  private: void UpdatePose();

  // Update the device data (the data going back to the client).
  private: void UpdateData();

  // Geometry of underlying position device.
  private: player_position_geom_t geom;

  // Compass setting (0 = off, 1 = partial, 2 = full).
  private: int compass;
  
  // Serial port.
  private: const char *port;

  // Position device info (the one we are subscribed to).
  private: int position_index;
  private: Driver *position;
  private: double position_time;
  private: double position_old_pose[3];
  private: double position_new_pose[3];

  // Handle to the imu tracker.
  private: ISD_TRACKER_HANDLE imu;
  private: double imu_old_orient;
  private: double imu_new_orient;

  // Combined pose estimate.
  private: double pose[3];
};


// Initialization function
Driver* InertiaCube2_Init( ConfigFile* cf, int section)
{
  if (strcmp( PLAYER_POSITION_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"inertiacube2\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }
  return ((Driver*) (new InertiaCube2( cf, section)));
}


// a driver registration function
void InertiaCube2_Register(DriverTable* table)
{
  table->AddDriver("inertiacube2", PLAYER_READ_MODE, InertiaCube2_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
InertiaCube2::InertiaCube2( ConfigFile* cf, int section)
    : Driver(cf, section, sizeof(player_position_data_t), 0, 10, 10)
{
  this->port = cf->ReadString(section, "port", "/dev/ttyS3");
  
  this->position_index = cf->ReadInt(section, "position_index", 0);
  this->position = NULL;
  this->position_time = 0;

  this->compass = cf->ReadInt(section, "compass", 2);
                              
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int InertiaCube2::Setup()
{
  // Initialise the underlying position device.
  if (this->SetupPosition() != 0)
    return -1;
  
  // Initialise the cube.
  if (this->SetupImu() != 0)
    return -1;

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int InertiaCube2::Shutdown()
{
  // Stop the driver thread.
  this->StopThread();

  // Stop the imu.
  this->ShutdownImu();

  // Stop the position device.
  this->ShutdownPosition();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying position device.
int InertiaCube2::SetupPosition()
{
  player_device_id_t id;

  id.code = PLAYER_POSITION_CODE;
  id.index = this->position_index;
  id.port = this->device_id.port;

  // Subscribe to the position device.
  this->position = deviceTable->GetDriver(id);
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
int InertiaCube2::ShutdownPosition()
{
  // Unsubscribe from devices.
  this->position->Unsubscribe(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize the imu.
int InertiaCube2::SetupImu()
{
  int i;
  int port;
  int verbose;
  ISD_TRACKER_INFO_TYPE info;
  ISD_STATION_INFO_TYPE sinfo;
  ISD_TRACKER_DATA_TYPE data;
  
  verbose = 0;
  
  // Open the tracker.  Takes a port number, so we strip it from the
  // port string.
  port = atoi(this->port + strlen(this->port) - 1);
  this->imu = ISD_OpenTracker((Hwnd) NULL, port + 1, FALSE, verbose);
  if (this->imu < 1)
  {
    PLAYER_ERROR("failed to detect InterSense tracking device");
    return -1;
  }

  // Get tracker configuration info
  if (!ISD_GetTrackerConfig(this->imu, &info, verbose))
  {
    PLAYER_ERROR("failed to get configuration info");
    return -1;
  }

  printf("InterSense Tracker type [%s] model [%s]\n",
         this->ImuType(info.TrackerType), this->ImuModel(info.TrackerModel));

  // Get some more configuration info.
  if (!ISD_GetStationConfig(this->imu, &sinfo, 1, verbose))
  {
    PLAYER_ERROR("failed to get station info");
    return -1;
  }

  // Set compass value (0 = off, 1 = partial, 2 = full).
  sinfo.Compass = this->compass;

  printf("compass %d enhancement %d sensitivity %d prediction %d format %d\n",
         sinfo.Compass, sinfo.Enhancement, sinfo.Sensitivity,
         sinfo.Prediction, sinfo.AngleFormat);

  // Change the configuration.
  if (!ISD_SetStationConfig(this->imu, &sinfo, 1, verbose))
  {
    PLAYER_ERROR("failed to get station info");
    return -1;
  }

  // Wait a while for the unit to settle.
  for (i = 0; i < 100;)
  {
    if (!ISD_GetData(this->imu, &data))
    {
      PLAYER_ERROR("failed to get data");
      return -1;
    }
    if (data.Station[0].NewData)
      i++;
    usleep(10000);
  }

  // Reset the heading component.
  if (!ISD_ResetHeading(this->imu, 1))
  {
    PLAYER_ERROR("failed to reset heading");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Finalize the imu.
int InertiaCube2::ShutdownImu()
{
  ISD_CloseTracker(this->imu);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the tracker type.
const char *InertiaCube2::ImuType(int type) 
{
  switch (type) 
  {
    case ISD_NONE:
      return "Unknown";
    case ISD_PRECISION_SERIES:
      return "IS Precision Series";
    case ISD_INTERTRAX_SERIES:
      return "InterTrax Series";
  }
  return "Unknown";
}


////////////////////////////////////////////////////////////////////////////////
// Get the tracker model.
const char *InertiaCube2::ImuModel(int model) 
{
  switch (model) 
  {
    case ISD_IS300:
      return "IS-300 Series";
    case ISD_IS600:
      return "IS-600 Series";
    case ISD_IS900:
      return "IS-900 Series";
    case ISD_INTERTRAX:
      return "InterTrax 30";
    case ISD_INTERTRAX_2:
      return "InterTrax2";
    case ISD_INTERTRAX_LS:
      return "InterTraxLS";
    case ISD_INTERTRAX_LC:
      return "InterTraxLC";
    case ISD_ICUBE2:
      return "InertiaCube2";
    case ISD_ICUBE2_PRO:
      return "InertiaCube2 Pro";
    case ISD_IS1200:
      return "IS-1200 Series";
  }
  return "Unknown";
}

 
////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void InertiaCube2::Main() 
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

    // Update the InertiaCube
    UpdateImu();

    // See if there is any new position data.  If there is, generate a
    // new pose estimate.
    if (UpdatePosition())
    {
      // Generate a new pose estimate.
      UpdatePose();

      // TESTING
      printf("%.3f %.3f %.0f  :  ",
             this->position_new_pose[0],
             this->position_new_pose[1],
             this->position_new_pose[2] * 180 / M_PI);
      printf("%.3f %.3f %.0f            \r",
             this->pose[0],
             this->pose[1],
             this->pose[2] * 180 / M_PI);
      
      // Expose the new estimate to the server.
      UpdateData();
    }
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int InertiaCube2::HandleRequests()
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
void InertiaCube2::HandleGetGeom(void *client, void *request, int len)
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
// Update the InertiaCube.
void InertiaCube2::UpdateImu()
{
  ISD_TRACKER_DATA_TYPE data;
  
  // Update the tracker data.
  if (ISD_GetData(this->imu, &data) == 0)
  {
    PLAYER_ERROR("error getting data");
    return;
  }

  // Pick out the yaw value.
  this->imu_new_orient = -data.Station[0].Orientation[0] * M_PI / 180;

  /*
  printf("orientation %f %f %f\r",
         data.Station[0].Orientation[0],
         data.Station[0].Orientation[1],
         data.Station[0].Orientation[2]);
  */

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the position device (returns non-zero if changed).
int InertiaCube2::UpdatePosition()
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
// Generate a new pose estimate.
// This algorithm assumes straight line segments.  We can probably to better
// than this.
void InertiaCube2::UpdatePose()
{
  double dx, dy, da;
  double tx, ty;

  // Compute change in pose relative to previous pose.
  dx = this->position_new_pose[0] - this->position_old_pose[0];
  dy = this->position_new_pose[1] - this->position_old_pose[1];
  da = this->position_old_pose[2];
  tx =  dx * cos(da) + dy * sin(da);
  ty = -dx * sin(da) + dy * cos(da);

  // Add this to the previous pose esimate.
  this->pose[0] += tx * cos(this->imu_old_orient) - ty * sin(this->imu_old_orient);
  this->pose[1] += tx * sin(this->imu_old_orient) + ty * cos(this->imu_old_orient);
  this->pose[2] = this->imu_new_orient;

  this->position_old_pose[0] = this->position_new_pose[0];
  this->position_old_pose[1] = this->position_new_pose[1];
  this->position_old_pose[2] = this->position_new_pose[2];
  this->imu_old_orient = this->imu_new_orient;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void InertiaCube2::UpdateData()
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
