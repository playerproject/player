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

#define PLAYER_ENABLE_TRACE 1

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#include "pf/pf.h"
#include "map/map.h"
#include "models/odometry.h"
#include "models/laser.h"

#ifdef INCLUDE_RTKGUI
#include "rtk.h"
#endif


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

  // Get the current pose
  private: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                  uint32_t* tsec, uint32_t* tusec);

  // Main function for device thread.
  private: virtual void Main();

  // Check for new odometry data
  private: int GetOdom();

  // Check for new laser data
  private: int GetLaser();

  // Initialize the filter
  private: void InitFilter(pf_vector_t pose_mean, pf_matrix_t pose_cov);

  // Update the filter with new sensor data
  private: void UpdateFilter();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *request, int len);

  // Handle map info request
  private: void HandleGetMapInfo(void *client, void *request, int len);

  // Handle map data request
  private: void HandleGetMapData(void *client, void *request, int len);

  // Odometry device info
  private: CDevice *odom;
  private: int odom_index;
  private: int odom_time_sec, odom_time_usec;

  // Current odometric pose
  private: pf_vector_t odom_pose;

  // Laser device info
  private: CDevice *laser;
  private: int laser_index;
  private: int laser_time_sec, laser_time_usec;

  // Laser pose relative to robot
  private: pf_vector_t laser_pose;

  // Laser range and bearing values
  private: int laser_count;
  private: double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];

  // Effective robot radius (used for c-space tests)
  private: double robot_radius;

  // Occupancy map
  private: const char *map_file;
  private: double map_scale;
  private: map_t *map;

  // Odometry sensor model
  private: odometry_t *odom_model;

  // Laser sensor model
  private: laser_t *laser_model;

  // Particle filter
  private: pf_t *pf;

  // Odometric pose at last filter update
  private: pf_vector_t pf_odom_pose;

  // Current particle filter pose estimate
  private: pf_vector_t pf_pose_mean;
  private: pf_matrix_t pf_pose_cov;

#ifdef INCLUDE_RTKGUI

  // RTK stuff; for testing only
  private: rtk_app_t *app;
  private: rtk_canvas_t *canvas;
  private: rtk_fig_t *map_fig;
  private: rtk_fig_t *pf_fig;
  
#endif
};


// Initialization function
CDevice* AdaptiveMCL_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_LOCALIZE_STRING) != 0)
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
    : CDevice(sizeof(player_localize_data_t), 0, 10, 10)
{
  double size;
  double u[3];
  
  this->odom = NULL;
  this->odom_index = cf->ReadInt(section, "position_index", 0);
  this->odom_time_sec = -1;
  this->odom_time_usec = -1;
  this->odom_pose = pf_vector_zero();

  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", 0);
  this->laser_time_sec = -1;
  this->laser_time_usec = -1;

  // TESTING; should get this by querying the laser geometry
  this->laser_pose.v[0] = cf->ReadTupleLength(section, "laser_pose", 0, 0);
  this->laser_pose.v[1] = cf->ReadTupleLength(section, "laser_pose", 1, 0);
  this->laser_pose.v[2] = cf->ReadTupleAngle(section, "laser_pose", 2, 0);

  // C-space info
  this->robot_radius = cf->ReadLength(section, "robot_radius", 0.20);
  
  // Get the map settings
  this->map_scale = cf->ReadLength(section, "map_scale", 0.05);
  this->map_file = cf->ReadFilename(section, "map_file", NULL);
  this->map = NULL;

  // TESTING
  this->map_file = "phe200-exp12-fly-05cm.pgm";
  
  // Odometry model settings
  this->odom_model = NULL;

  // Laser model settings
  this->laser_model = NULL;

  // Particle filter settings
  this->pf = NULL;
  this->pf_odom_pose = pf_vector_zero();

  // Initial pose estimate
  this->pf_pose_mean = pf_vector_zero();
  this->pf_pose_mean.v[0] = cf->ReadTupleLength(section, "init_pose", 0, 0);
  this->pf_pose_mean.v[1] = cf->ReadTupleLength(section, "init_pose", 1, 0);
  this->pf_pose_mean.v[2] = cf->ReadTupleAngle(section, "init_pose", 2, 0);

  // Initial pose covariance
  u[0] = cf->ReadTupleLength(section, "init_pose_var", 0, 1e2);
  u[1] = cf->ReadTupleLength(section, "init_pose_var", 1, 1e2);
  u[2] = cf->ReadTupleAngle(section, "init_pose_var", 2, 1e2);
  this->pf_pose_cov = pf_matrix_zero();
  this->pf_pose_cov.m[0][0] = u[0] * u[0];
  this->pf_pose_cov.m[1][1] = u[1] * u[1];
  this->pf_pose_cov.m[2][2] = u[2] * u[2];

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
  PLAYER_TRACE("setup");
  
  // Initialise the underlying position device.
  if (this->SetupOdom() != 0)
    return -1;
  
  // Initialise the laser.
  if (this->SetupLaser() != 0)
    return -1;

  // Create the map
  if (!this->map_file)
  {
    PLAYER_ERROR("map file not specified");
    return -1;
  }
  
  // Create the map
  assert(this->map == NULL);
  this->map = map_alloc(this->map_scale);

  // Load the map
  if (map_load_occ(this->map, this->map_file) != 0)
    return -1;

  // Compute the c-space
  map_update_dist(this->map, 2 * this->robot_radius);

  // Create the odometry model
  this->odom_model = odometry_alloc(this->map, this->robot_radius);

  // Create the laser model
  this->laser_model = laser_alloc(this->map);

  // Create the particle filter
  assert(this->pf == NULL);
  this->pf = pf_alloc(100, 200000);
  
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

  // Delete the particle filter
  pf_free(this->pf);
  this->pf = NULL;

  // Delete the odometry model
  odometry_free(this->odom_model);
  this->odom_model = NULL;

  // Delete the laser model
  laser_free(this->laser_model);
  this->laser_model = NULL;
  
  // Delete the map
  map_free(this->map);
  this->map = NULL;
  
  // Stop the laser
  this->ShutdownLaser();

  // Stop the odom device.
  this->ShutdownOdom();

  PLAYER_TRACE("shutdown");
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
// Get the current pose
size_t AdaptiveMCL::GetData(void* client, unsigned char* dest, size_t len,
                            uint32_t* tsec, uint32_t* tusec)
{
  int i, j, k;
  int datalen;
  player_localize_data_t data;
  pf_vector_t pose_mean;
  pf_matrix_t pose_cov;
  
  this->Lock();

  // Get the current estimate
  pose_mean = this->pf_pose_mean;
  pose_cov = this->pf_pose_cov;
  
  // Translate the hypotheses
  // TODO

  // Encode the one-and-only hypothesis
  data.hypoth_count = 1;

  data.hypoths[0].mean[0] = (int32_t) (pose_mean.v[0] * 1000);
  data.hypoths[0].mean[1] = (int32_t) (pose_mean.v[0] * 1000);
  data.hypoths[0].mean[2] = (int32_t) (pose_mean.v[2] * 180 * 3600 / M_PI);
  
  data.hypoths[0].cov[0][0] = (int32_t) (pose_cov.m[0][0] * 1000 * 1000);
  data.hypoths[0].cov[0][1] = (int32_t) (pose_cov.m[0][1] * 1000 * 1000);
  data.hypoths[0].cov[0][2] = 0;
  
  data.hypoths[0].cov[1][0] = (int32_t) (pose_cov.m[1][0] * 1000 * 1000);
  data.hypoths[0].cov[1][1] = (int32_t) (pose_cov.m[1][1] * 1000 * 1000);
  data.hypoths[0].cov[1][2] = 0;

  data.hypoths[0].cov[2][0] = 0;
  data.hypoths[0].cov[2][1] = 0;
  data.hypoths[0].cov[2][2] = (int32_t) (pose_cov.m[2][2] * 180 * 3600 / M_PI * 180 * 3600 / M_PI);

  data.hypoths[0].alpha = 0;

  this->Unlock();
  
  // Compute the length of the data packet
  datalen = sizeof(data) - sizeof(data.hypoths) + data.hypoth_count * sizeof(data.hypoths[0]);

  // Byte-swap
  for (i = 0; i < data.hypoth_count; i++)
  {
    for (j = 0; j < 3; j++)
    {
      data.hypoths[i].mean[j] = htonl(data.hypoths[i].mean[j]);
      for (k = 0; k < 3; k++)
        data.hypoths[i].cov[j][k] = htonl(data.hypoths[i].cov[j][k]);
    }
    data.hypoths[i].alpha = htons(data.hypoths[i].alpha);
  }
  data.hypoth_count = htonl(data.hypoth_count);

  // Copy data to server
  assert(len >= datalen);
  memcpy(dest, &data, datalen);

  // Set data timestamp
  *tsec = this->odom_time_sec;
  *tusec = this->odom_time_usec;
  
  return datalen;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void AdaptiveMCL::Main() 
{
  struct timespec sleeptime;

#ifdef INCLUDE_RTKGUI

  rtk_init(NULL, NULL);

  // Create the gui
  this->app = rtk_app_create();

  this->canvas = rtk_canvas_create(this->app);
  rtk_canvas_title(this->canvas, "AdaptiveMCL");
  rtk_canvas_size(this->canvas, this->map->size_x, this->map->size_y);
  rtk_canvas_scale(this->canvas, this->map->scale, this->map->scale);

  this->map_fig = rtk_fig_create(this->canvas, NULL, -1);
  this->pf_fig = rtk_fig_create(this->canvas, this->map_fig, 0);
  
  // Draw the map
  //map_draw_occ(this->map, this->map_fig);
  map_draw_cspace(this->map, this->map_fig);

  rtk_app_main_init(this->app);
  
#endif
  
  // WARNING: this only works for Linux
  // Run at a lower priority
  nice(10);

  // Initialize the filter
  this->InitFilter(this->pf_pose_mean, this->pf_pose_cov);
  
  while (true)
  {
#ifdef INCLUDE_RTKGUI
    rtk_canvas_render(this->canvas);
    rtk_app_main_loop(this->app);
#endif

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

  
#ifdef INCLUDE_RTKGUI
  rtk_app_main_term(this->app);
  rtk_canvas_destroy(this->canvas);
  rtk_app_destroy(this->app);
#endif

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new odometry data
int AdaptiveMCL::GetOdom()
{
  int i;
  size_t size;
  player_position_data_t data;
  uint32_t time_sec, time_usec;
  double time;
  
  // Get the odom device data.
  size = this->odom->GetData(this,(unsigned char*) &data, sizeof(data), &time_sec, &time_usec);

  // Dont do anything if this is old data.
  if (time_sec == this->odom_time_sec && time_usec == this->odom_time_usec)
    return 0;
  this->odom_time_sec = time_sec;
  this->odom_time_usec = time_usec;

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
int AdaptiveMCL::GetLaser()
{
  int i;
  size_t size;
  player_laser_data_t data;
  uint32_t time_sec, time_usec;
  double r, b, db;
  
  // Get the laser device data.
  size = this->laser->GetData(this,(unsigned char*) &data, sizeof(data), &time_sec, &time_usec);

  // Dont do anything if this is old data.
  if (time_sec == this->laser_time_sec && time_usec == this->laser_time_usec)
    return 0;
  this->laser_time_sec = time_sec;
  this->laser_time_usec = time_usec;

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
// Initialize the filter
void AdaptiveMCL::InitFilter(pf_vector_t pose_mean, pf_matrix_t pose_cov)
{
  // Initialize the odometric model
  odometry_init_pose(this->odom_model, pose_mean, pose_cov);

  // Draw samples from the odometric distribution
  pf_init(this->pf, (pf_init_model_fn_t) odometry_init_model, this->odom_model);

#ifdef INCLUDE_RTKGUI
  // Draw the samples
  rtk_fig_clear(this->pf_fig);
  pf_draw_samples(this->pf, this->pf_fig, 1000);
#endif
  
  this->Lock();
  
  // Re-compute the pose estimate
  pf_calc_stats(this->pf, &this->pf_pose_mean, &this->pf_pose_cov);

  PLAYER_TRACE("pf: %f %f %f",
               this->pf_pose_mean.v[0], this->pf_pose_mean.v[1], this->pf_pose_mean.v[2]);
  
  this->Unlock();

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the filter with new sensor data
void AdaptiveMCL::UpdateFilter()
{
  pf_vector_t diff;

  // Wait until we have valid data
  if (this->odom_time_sec < 0)
    return;
  if (this->laser_time_sec < 0)
    return;
  
  // Compute change in pose since last update
  diff = pf_vector_coord_sub(this->odom_pose, this->pf_odom_pose);

  // Make sure we have moved a reasonable distance
  if (fabs(diff.v[0]) < 0.20 && fabs(diff.v[1]) < 0.20 && fabs(diff.v[2]) < M_PI / 6)
    return;

  // Set odometry sensor readings
  odometry_set_pose(this->odom_model, this->pf_odom_pose, this->odom_pose);  
  odometry_set_stall(this->odom_model, 0);

  // Apply the odometry action model
  pf_update_action(this->pf, (pf_action_model_fn_t) odometry_action_model, this->odom_model);

  // Apply the odometry sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) odometry_sensor_model, this->odom_model);

  // Resample
  pf_update_resample(this->pf);

#ifdef INCLUDE_RTKGUI
  // Draw the samples
  rtk_fig_clear(this->pf_fig);
  pf_draw_samples(this->pf, this->pf_fig, 1000);
#endif

  this->Lock();
  
  // Re-compute the pose estimate
  pf_calc_stats(this->pf, &this->pf_pose_mean, &this->pf_pose_cov);

  PLAYER_TRACE("pf: %f %f %f",
               this->pf_pose_mean.v[0], this->pf_pose_mean.v[1], this->pf_pose_mean.v[2]);
  
  this->pf_odom_pose = this->odom_pose;

  this->Unlock();
  
  return;
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

      case PLAYER_LOCALIZE_GET_MAP_INFO_REQ:
        HandleGetMapInfo(client, request, len);
        break;

      case PLAYER_LOCALIZE_GET_MAP_DATA_REQ:
        HandleGetMapData(client, request, len);
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



////////////////////////////////////////////////////////////////////////////////
// Handle map info request
void AdaptiveMCL::HandleGetMapInfo(void *client, void *request, int len)
{
  size_t reqlen;
  player_localize_map_info_t info;

  // Expected length of request
  reqlen = sizeof(info.subtype);
  
  // check if the config request is valid
  if (len != reqlen)
  {
    PLAYER_ERROR("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }
  
  info.scale = htonl((int32_t) (1000.0 / this->map->scale + 0.5));
  info.width = htonl((int32_t) (this->map->size_x + 0.5));
  info.height = htonl((int32_t) (this->map->size_y + 0.5));

  PLAYER_TRACE("%f %d\n", this->map->scale, ntohl(info.scale));

  // Send map info to the client
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &info, sizeof(info)) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Handle map data request
void AdaptiveMCL::HandleGetMapData(void *client, void *request, int len)
{
  int i, j;
  int oi, oj, si, sj;
  size_t reqlen;
  map_cell_t *cell;
  player_localize_map_data_t data;

  // Expected length of request
  reqlen = sizeof(data) - sizeof(data.data);

  // check if the config request is valid
  if (len != reqlen)
  {
    PLAYER_ERROR("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  // Construct reply
  memcpy(&data, request, len);

  oi = ntohl(data.col);
  oj = ntohl(data.row);
  si = ntohl(data.width);
  sj = ntohl(data.height);

  PLAYER_TRACE("%d %d %d %d\n", oi, oj, si, sj);

  // Grab the pixels from the map
  for (j = 0; j < sj; j++)
  {
    for (i = 0; i < si; i++)
    {
      if (MAP_VALID(this->map, i + oi, j + oj))
      {
        cell = this->map->cells + MAP_INDEX(this->map, i + oi, j + oj);
        data.data[i + j * si] = cell->occ_state;
      }
      else
        data.data[i + j * si] = 0;
    }
  }
    
  // Send map info to the client
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &data, sizeof(data)) != 0)
    PLAYER_ERROR("PutReply() failed");
  
  return;
}
