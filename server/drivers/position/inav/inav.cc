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

#include "imap/imap.h"
#include "inav_vector.h"


// Incremental navigation driver
class INav : public CDevice
{
  // Constructor
  public: INav(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~INav();

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();
  
  // Set the driver command
  public: virtual void PutCommand(void* client, unsigned char* src, size_t len);
  
  // Set up the odometry device.
  private: int SetupOdom();
  private: int ShutdownOdom();

  // Set up the laser device.
  private: int SetupLaser();
  private: int ShutdownLaser();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Check for new odometry data
  private: int GetOdom();

  // Check for new laser data
  private: int GetLaser();

  // Write the pose data (the data going back to the client).
  private: void PutPose();

  // Update the incremental pose in response to new laser data.
  private: void UpdatePoseLaser();

  // Odometry device info
  private: CDevice *odom;
  private: int odom_index;
  private: double odom_time;

  // Pose of robot in odometric cs
  private: inav_vector_t odom_pose;

  // Velocity of robot in robot cs
  private: inav_vector_t odom_vel;
  
  // Laser device info
  private: CDevice *laser;
  private: int laser_index;
  private: double laser_time;

  // Pose of laser in robot cs
  private: inav_vector_t laser_pose;

  // Laser range and bearing values
  private: int laser_count;
  private: double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];

  // Current incremental pose estimate
  private: inav_vector_t inc_pose;

  // Odometric pose used in last update
  private: inav_vector_t inc_odom_pose;

  // Incremental occupancy map
  private: imap_t *map;
  private: double map_scale;
  private: inav_vector_t map_pose;
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
  double size;
  
  this->odom = NULL;
  this->odom_index = cf->ReadInt(section, "position_index", 0);
  this->odom_time = 0.0;

  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", 0);
  this->laser_time = 0.0;

  // TESTING; should get this by querying the laser geometry
  this->laser_pose.v[0] = cf->ReadTupleLength(section, "laser_pose", 0, 0);
  this->laser_pose.v[1] = cf->ReadTupleLength(section, "laser_pose", 1, 0);
  this->laser_pose.v[2] = cf->ReadTupleAngle(section, "laser_pose", 2, 0);
  
  this->inc_pose.v[0] = 0.0;
  this->inc_pose.v[1] = 0.0;
  this->inc_pose.v[2] = 0.0;

  this->inc_odom_pose.v[0] = 0.0;
  this->inc_odom_pose.v[1] = 0.0;
  this->inc_odom_pose.v[2] = 0.0;

  size = cf->ReadLength(section, "map_size", 16.0);
  this->map_scale = cf->ReadLength(section, "map_scale", 0.10);
  
  this->map = imap_alloc((int) (size / this->map_scale),
                         (int) (size / this->map_scale), this->map_scale, 0.25, 0.25);

  this->map_pose.v[0] = 0.0;
  this->map_pose.v[1] = 0.0;
  this->map_pose.v[2] = 0.0;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
INav::~INav()
{
  imap_free(this->map);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int INav::Setup()
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
int INav::Shutdown()
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
// Set the driver command
void INav::PutCommand(void* client, unsigned char* src, size_t len)
{
  // TODO: implement local goto controller
  // Pass command to odometry device
  this->odom->PutCommand(client, src, len);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int INav::SetupOdom()
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
int INav::ShutdownOdom()
{
  this->odom->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int INav::SetupLaser()
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

  // Clear the map
  imap_reset(this->map);

  while (true)
  {
    // Sleep for 1ms (will actually take longer than this).
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000L;
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    HandleRequests();

    // Check for new odometric data
    GetOdom();
    
    // Check for new laser data.  If there is new data, update the
    // incremental pose estimate.
    if (GetLaser())
    {
      UpdatePoseLaser();
      PutPose();
    }
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
void INav::HandleGetGeom(void *client, void *request, int len)
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


////////////////////////////////////////////////////////////////////////////////
// Check for new odometry data
int INav::GetOdom()
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

  this->odom_pose.v[0] = (double) ((int32_t) data.xpos) / 1000.0;
  this->odom_pose.v[1] = (double) ((int32_t) data.ypos) / 1000.0;
  this->odom_pose.v[2] = (double) ((int32_t) data.yaw) * M_PI / 180;
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new laser data
int INav::GetLaser()
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
// Update the device data (the data going back to the client).
void INav::PutPose()
{
  uint32_t timesec, timeusec;
  player_position_data_t data;

  // Pose esimate
  data.xpos = (int32_t) (this->inc_pose.v[0] * 1000);
  data.ypos = (int32_t) (this->inc_pose.v[1] * 1000);
  data.yaw = (int32_t) (this->inc_pose.v[2] * 180 / M_PI);

  // Velocity estimate (use odometry device's pose esimate)
  data.xspeed = (int32_t) (this->odom_vel.v[0] * 1000);
  data.yspeed = (int32_t) (this->odom_vel.v[1] * 1000);
  data.yawspeed = (int32_t) (this->odom_vel.v[2] * 180 / M_PI);

  // Byte swap
  data.xpos = htonl(data.xpos);
  data.ypos = htonl(data.ypos);
  data.yaw = htonl(data.yaw);
  data.xspeed = htonl(data.xspeed);
  data.yspeed = htonl(data.yspeed);
  data.yawspeed = htonl(data.yawspeed);

  // Compute time.  Use the laser device's time.
  timesec = (uint32_t) this->laser_time;
  timeusec = (uint32_t) (fmod(this->laser_time, 1.0) * 1e6);

  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the incremental pose in response to new laser data.
void INav::UpdatePoseLaser()
{
  int di, dj;
  inav_vector_t d, pose;

  // Compute new incremental pose
  d = inav_vector_cs_sub(this->odom_pose, this->inc_odom_pose);
  this->inc_pose = inav_vector_cs_add(d, this->inc_pose);
  this->inc_odom_pose = this->odom_pose;

  // Translate the map if we stray from the center
  d = inav_vector_cs_sub(this->inc_pose, this->map_pose);
  di = (int) (d.v[0] / this->map_scale);
  dj = (int) (d.v[1] / this->map_scale);
  if (abs(di) > 0 || abs(dj) > 0)
  {
    imap_translate(this->map, di, dj);
    this->map_pose.v[0] += di * this->map_scale;
    this->map_pose.v[1] += dj * this->map_scale;
  }

  // Compute the best fit between the laser scan and the map.
  pose = this->inc_pose;
  imap_fit_ranges(this->map, pose.v, this->laser_pose.v,
                  this->laser_count, this->laser_ranges);
  this->inc_pose = pose;
    
  // Update the map with the current range readings
  imap_add_ranges(this->map, this->inc_pose.v, this->laser_pose.v,
                  this->laser_count, this->laser_ranges);

  /*
  // TESTING
  static int count;
  char filename[64];
  snprintf(filename, sizeof(filename), "imap_%04d.pgm", count++);
  imap_save_occ(this->map, filename);
  */

  return;
}
