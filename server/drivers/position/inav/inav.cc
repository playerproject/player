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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#include "inav_vector.h"
#include "inav_map.h"
#include "inav_con.h"


#ifdef INCLUDE_RTKGUI
#include "rtk.h"
#endif


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
    
#ifdef INCLUDE_RTKGUI
  // Set up the GUI
  private: int SetupGUI();
  private: int ShutdownGUI();

  // Draw the map
  private: void DrawMap();

  // Draw the robot
  private: void DrawRobot();
#endif

  // Set up the odometry device.
  private: int SetupOdom();
  private: int ShutdownOdom();

  // Set up the laser device.
  private: int SetupLaser();
  private: int ShutdownLaser();

  // Main function for device thread.
  private: virtual void Main();

  // Update the incremental pose in response to new laser data.
  private: void UpdatePose();

  // Update controller
  private: void UpdateControl();

  // Check for new odometry data
  private: int GetOdom();

  // Check for new laser data
  private: int GetLaser();

  // Check for new commands from server
  private: void GetCommand();

  // Send commands to underlying position device
  private: void PutCommand();

  // Write the pose data (the data going back to the client).
  private: void PutPose();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);

  // Handle motor power requests
  private: void HandlePower(void *client, void *req, int reqlen);

  // Odometry device info
  private: CDevice *odom;
  private: int odom_index;
  private: double odom_time;

  // Odometric geometry (robot size and pose in robot cs)
  private: inav_vector_t odom_geom_pose;
  private: inav_vector_t odom_geom_size;

  // Pose of robot in odometric cs
  private: inav_vector_t odom_pose;

  // Velocity of robot in robot cs
  private: inav_vector_t odom_vel;
  
  // Laser device info
  private: CDevice *laser;
  private: int laser_index;
  private: double laser_time;

  // Laser geometry (pose of laser in robot cs)
  private: inav_vector_t laser_geom_pose;

  // Laser range and bearing values
  private: int laser_count;
  private: double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];
  
  // Current incremental pose estimate
  private: inav_vector_t inc_pose;
  private: inav_vector_t inc_vel;

  // Odometric pose used in last update
  private: inav_vector_t inc_odom_pose;

  // Incremental occupancy map
  private: imap_t *map;
  private: double map_scale;
  private: inav_vector_t map_pose;

  // Local controller
  private: icon_t *con;
  private: inav_vector_t goal_pose;
  private: inav_vector_t con_vel;
  
#ifdef INCLUDE_RTKGUI
  // RTK stuff; for testing only
  private: rtk_app_t *app;
  private: rtk_canvas_t *canvas;
  private: rtk_fig_t *map_fig;
  private: rtk_fig_t *robot_fig;
  private: rtk_fig_t *path_fig;
#endif

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

  // The actual odometry geometry is read from the odometry device
  this->odom_geom_pose = inav_vector_zero();
  this->odom_geom_size = inav_vector_zero();

  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", 0);
  this->laser_time = 0.0;

  // The actual laser geometry is read from the laser device
  this->laser_geom_pose = inav_vector_zero();

  // Laser settings
  //TODO this->laser_max_samples = cf->ReadInt(section, "laser_max_samples", 10);
  
  this->inc_pose = inav_vector_zero();
  this->inc_odom_pose = inav_vector_zero();

  // Load map settings
  size = cf->ReadLength(section, "map_size", 16.0);
  this->map_scale = cf->ReadLength(section, "map_scale", 0.10);

  // Create the map
  this->map = imap_alloc((int) (size / this->map_scale), (int) (size / this->map_scale),
                         this->map_scale, 0.30, 0.20);

  this->map_pose = inav_vector_zero();

  // Create the controller
  this->con = icon_alloc(this->map, 0.30);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
INav::~INav()
{
  icon_free(this->con);
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
  
#ifdef INCLUDE_RTKGUI
  // Start the GUI
  this->SetupGUI();
#endif

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

#ifdef INCLUDE_RTKGUI
  // Stop the GUI
  this->ShutdownGUI();
#endif

  // Stop the laser
  this->ShutdownLaser();

  // Stop the odom device.
  this->ShutdownOdom();

  return 0;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Set up the GUI
int INav::SetupGUI()
{
  // Initialize RTK
  rtk_init(NULL, NULL);

  this->app = rtk_app_create();

  this->canvas = rtk_canvas_create(this->app);
  rtk_canvas_title(this->canvas, "IncrementalNav");
  rtk_canvas_size(this->canvas, this->map->size_x * 2, this->map->size_y * 2);
  rtk_canvas_scale(this->canvas, this->map->scale / 2, this->map->scale / 2);

  this->map_fig = rtk_fig_create(this->canvas, NULL, -1);
  this->robot_fig = rtk_fig_create(this->canvas, NULL, 0);
  this->path_fig = rtk_fig_create(this->canvas, NULL, 1);
  
  rtk_app_main_init(this->app);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the GUI
int INav::ShutdownGUI()
{
  rtk_fig_destroy(this->path_fig);
  rtk_fig_destroy(this->robot_fig);
  rtk_fig_destroy(this->map_fig);

  rtk_canvas_destroy(this->canvas);
  rtk_app_main_term(this->app);
  rtk_app_destroy(this->app);
  
  return 0;
}
  
#endif


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int INav::SetupOdom()
{
  uint8_t req;
  size_t replen;
  unsigned short reptype;
  struct timeval ts;
  player_position_geom_t geom;
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

  // Get the odometry geometry
  req = PLAYER_POSITION_GET_GEOM_REQ;
  replen = this->odom->Request(&this->odom->device_id, this, &req, sizeof(req),
                               &reptype, &ts, &geom, sizeof(geom));

  geom.pose[0] = ntohs(geom.pose[0]);
  geom.pose[1] = ntohs(geom.pose[1]);
  geom.pose[2] = ntohs(geom.pose[2]);
  geom.size[0] = ntohs(geom.size[0]);
  geom.size[1] = ntohs(geom.size[1]);

  this->odom_geom_pose.v[0] = (int16_t) geom.pose[0] / 1000.0;
  this->odom_geom_pose.v[1] = (int16_t) geom.pose[1] / 1000.0;
  this->odom_geom_pose.v[2] = (int16_t) geom.pose[2] / 180 * M_PI;
  
  this->odom_geom_size.v[0] = (int16_t) geom.size[0] / 1000.0;
  this->odom_geom_size.v[1] = (int16_t) geom.size[1] / 1000.0;

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
  uint8_t req;
  size_t replen;
  unsigned short reptype;
  struct timeval ts;
  player_laser_geom_t geom;
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
  
  // Get the laser geometry
  req = PLAYER_LASER_GET_GEOM;
  replen = this->laser->Request(&this->laser->device_id, this, &req, sizeof(req),
                                &reptype, &ts, &geom, sizeof(geom));

  geom.pose[0] = ntohs(geom.pose[0]);
  geom.pose[1] = ntohs(geom.pose[1]);
  geom.pose[2] = ntohs(geom.pose[2]);
  geom.size[0] = ntohs(geom.size[0]);
  geom.size[1] = ntohs(geom.size[1]);

  this->laser_geom_pose.v[0] = (int16_t) geom.pose[0] / 1000.0;
  this->laser_geom_pose.v[1] = (int16_t) geom.pose[1] / 1000.0;
  this->laser_geom_pose.v[2] = (int16_t) geom.pose[2] / 180 * M_PI;
  
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
  int update;
  int map_update, robot_update;
  struct timespec sleeptime;

  // Clear the update counter
  update = 0;
  map_update = 0;
  robot_update = 0;
  
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

#ifdef INCLUDE_RTKGUI
    // Re-draw the map occasionally
    if (update - map_update >= 10)
    {
      this->DrawMap();
      map_update = update;
      rtk_canvas_render(this->canvas);      
    }
                     
    // Re-draw the robot frequently
    if (update - robot_update >= 1)
    {
      this->DrawRobot();
      robot_update = update;
      rtk_canvas_render(this->canvas);
    }

    rtk_app_main_loop(this->app);
#endif

    // Process any pending requests.
    this->HandleRequests();

    // Check for new commands
    this->GetCommand();
    
    // Check for new odometric data.  If there is new data, update the
    // controller.
    if (this->GetOdom())
    {
      this->UpdateControl();
      this->PutCommand();
    }
    
    // Check for new laser data.  If there is new data, update the
    // incremental pose estimate.
    if (this->GetLaser())
    {
      this->UpdatePose();
      this->PutPose();
      update++;
    }
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the incremental pose in response to new laser data.
void INav::UpdatePose()
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

  // TODO: dont use all laser values
  
  // Compute the best fit between the laser scan and the map.
  pose = this->inc_pose;
  imap_fit_ranges(this->map, pose.v, this->laser_geom_pose.v,
                  this->laser_count, this->laser_ranges);
  this->inc_pose = pose;
    
  // Update the map with the current range readings
  imap_add_ranges(this->map, this->inc_pose.v, this->laser_geom_pose.v,
                  this->laser_count, this->laser_ranges);

  // TODO: filter this
  // Estimate the robot velocity
  this->inc_vel = this->odom_vel;

  /*
  // TESTING
  static int count;
  char filename[64];
  snprintf(filename, sizeof(filename), "imap_%04d.pgm", count++);
  imap_save_occ(this->map, filename);
  */

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update controller
void INav::UpdateControl()
{
  // Set the goal pose
  icon_set_goal(this->con, this->goal_pose.v);
  
  // Set the current robot state
  icon_set_robot(this->con, this->inc_pose.v, this->inc_vel.v);

  // Compute the control velocities (robot cs)
  icon_get_control(this->con, this->con_vel.v);

  //printf("control vel %f %f %f\n",
  //       this->con_vel.v[0], this->con_vel.v[1], this->con_vel.v[2]);
  
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
  if (size == 0)
    return 0;
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (time - this->odom_time < 0.001)
    return 0;
  this->odom_time = time;

  // Byte swap
  data.xpos = ntohl(data.xpos);
  data.ypos = ntohl(data.ypos);
  data.yaw = ntohl(data.yaw);

  data.xspeed = ntohl(data.xspeed);
  data.yspeed = ntohl(data.yspeed);
  data.yawspeed = ntohl(data.yawspeed);

  this->odom_pose.v[0] = (double) ((int32_t) data.xpos) / 1000.0;
  this->odom_pose.v[1] = (double) ((int32_t) data.ypos) / 1000.0;
  this->odom_pose.v[2] = (double) ((int32_t) data.yaw) * M_PI / 180;

  this->odom_vel.v[0] = (double) ((int32_t) data.xspeed) / 1000.0;
  this->odom_vel.v[1] = (double) ((int32_t) data.yspeed) / 1000.0;
  this->odom_vel.v[2] = (double) ((int32_t) data.yawspeed) * M_PI / 180;

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
  if (size == 0)
    return 0;
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
// Check for new commands from the server
void INav::GetCommand()
{
  player_position_cmd_t cmd;

  if (CDevice::GetCommand(&cmd, sizeof(cmd)) == 0)
  {
    this->goal_pose = this->inc_pose;
  }
  else
  {
    this->goal_pose.v[0] = ((int32_t) ntohl(cmd.xpos)) / 1000.0;
    this->goal_pose.v[1] = ((int32_t) ntohl(cmd.ypos)) / 1000.0;
    this->goal_pose.v[2] = ((int32_t) ntohl(cmd.yaw)) / 180 * M_PI;
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Send commands to the underlying position device
void INav::PutCommand()
{
  player_position_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  
  cmd.xspeed = (int32_t) (this->con_vel.v[0] * 1000);
  cmd.yspeed = (int32_t) (this->con_vel.v[1] * 1000);
  cmd.yawspeed = (int32_t) (this->con_vel.v[2] * 180 / M_PI);

  cmd.xspeed = htonl(cmd.xspeed);
  cmd.yspeed = htonl(cmd.yspeed);
  cmd.yawspeed = htonl(cmd.yawspeed);

  this->odom->PutCommand(this, (unsigned char*) &cmd, sizeof(cmd));
  
  return;
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
  data.xspeed = (int32_t) (this->inc_vel.v[0] * 1000);
  data.yspeed = (int32_t) (this->inc_vel.v[1] * 1000);
  data.yawspeed = (int32_t) (this->inc_vel.v[2] * 180 / M_PI);

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
      case PLAYER_POSITION_GET_GEOM_REQ:
        HandleGetGeom(client, request, len);
        break;

      case PLAYER_POSITION_MOTOR_POWER_REQ:
        HandlePower(client, request, len);
        break;
        
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
void INav::HandleGetGeom(void *client, void *req, int reqlen)
{
  player_position_geom_t geom;

  geom.subtype = PLAYER_POSITION_GET_GEOM_REQ;
  geom.pose[0] = htons((int) (this->odom_geom_pose.v[0] * 1000));
  geom.pose[1] = htons((int) (this->odom_geom_pose.v[1] * 1000));
  geom.pose[2] = htons((int) (this->odom_geom_pose.v[2] * 180 / M_PI));
  geom.size[0] = htons((int) (this->odom_geom_size.v[0] * 1000));
  geom.size[1] = htons((int) (this->odom_geom_size.v[1] * 1000));

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &geom, sizeof(geom)) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle motor power requests
void INav::HandlePower(void *client, void *req, int reqlen)
{
  unsigned short reptype;
  struct timeval ts;

  this->odom->Request(&this->odom->device_id, this, req, reqlen, &reptype, &ts, NULL, 0);
  if (PutReply(client, reptype, &ts, NULL, 0) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Draw the map
void INav::DrawMap()
{
  rtk_fig_clear(this->map_fig);
  imap_draw_occ(this->map, this->map_fig);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the robot
void INav::DrawRobot()
{
  inav_vector_t pose;
    
  rtk_fig_clear(this->robot_fig);
  rtk_fig_color(this->robot_fig, 0.7, 0, 0);
  
  pose = this->inc_pose;
  rtk_fig_origin(this->robot_fig, pose.v[0], pose.v[1], pose.v[2]);

  // Draw the robot body
  pose = this->odom_geom_pose;
  rtk_fig_rectangle(this->robot_fig, pose.v[0], pose.v[1], pose.v[2],
                    this->odom_geom_size.v[0], this->odom_geom_size.v[1], 0);

  // Draw the predicted robot path
  rtk_fig_clear(this->path_fig);
  icon_draw(this->con, this->path_fig);
  
  return;
}

#endif
