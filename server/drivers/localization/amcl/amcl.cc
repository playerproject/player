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
// Requires: position (odometry), laser, sonar
// Provides: localization
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <sys/types.h>
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>
#include <sys/time.h>

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 0

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#include "pf/pf.h"
#include "map/map.h"
#include "models/odometry.h"
#include "models/sonar.h"
#include "models/laser.h"

#ifdef INCLUDE_RTKGUI
#include "rtk.h"
#endif


// Combined sensor data
typedef struct
{
  // Data time-stamp (odometric)
  uint32_t odom_time_sec, odom_time_usec;
    
  // Odometric pose
  pf_vector_t odom_pose;

  // Sonar ranges
  int srange_count;
  double sranges[PLAYER_SONAR_MAX_SAMPLES];

  // Laser ranges
  int range_count;
  double ranges[PLAYER_LASER_MAX_SAMPLES][2];
  
} amcl_sensor_data_t;


// Pose hypothesis
typedef struct
{
  // Total weight (weights sum to 1)
  double weight;

  // Mean of pose esimate
  pf_vector_t pf_pose_mean;

  // Covariance of pose estimate
  pf_matrix_t pf_pose_cov;
  
} amcl_hyp_t;


// Incremental navigation driver
class AdaptiveMCL : public CDevice
{
  ///////////////////////////////////////////////////////////////////////////
  // Top half methods; these methods run in the server thread
  ///////////////////////////////////////////////////////////////////////////

  // Constructor
  public: AdaptiveMCL(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~AdaptiveMCL(void);

  // Setup/shutdown routines.
  public: virtual int Setup(void);
  public: virtual int Shutdown(void);

#ifdef INCLUDE_RTKGUI
  // Set up the GUI
  private: int SetupGUI(void);
  private: int ShutdownGUI(void);
#endif

  // Set up the odometry device
  private: int SetupOdom(void);
  private: int ShutdownOdom(void);

  // Get the current odometric pose
  private: void GetOdomData(amcl_sensor_data_t *data);

  // Set up the sonar device
  private: int SetupSonar(void);
  private: int ShutdownSonar(void);

  // Check for new sonar data
  private: void GetSonarData(amcl_sensor_data_t *data);

  // Set up the laser device
  private: int SetupLaser(void);
  private: int ShutdownLaser(void);

  // Check for new laser data
  private: void GetLaserData(amcl_sensor_data_t *data);

  // Get the current pose
  private: virtual size_t GetData(void* client, unsigned char* dest, size_t len,
                                  uint32_t* time_sec, uint32_t* time_usec);

  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t* device, void* client, 
                                void* data, size_t len);

  // Handle map info request
  private: void HandleGetMapInfo(void *client, void *request, int len);

  // Handle map data request
  private: void HandleGetMapData(void *client, void *request, int len);

  ///////////////////////////////////////////////////////////////////////////
  // Middle methods: these methods facilitate communication between the top
  // and bottom halfs.
  ///////////////////////////////////////////////////////////////////////////

  // Push data onto the filter queue
  private: void Push(amcl_sensor_data_t *data);

  // Pop data from the filter queue
  private: int Pop(amcl_sensor_data_t *data);

  ///////////////////////////////////////////////////////////////////////////
  // Bottom half methods; these methods run in the device thread
  ///////////////////////////////////////////////////////////////////////////
  
  // Main function for device thread.
  private: virtual void Main(void);

  // Initialize the filter
  private: void InitFilter(pf_vector_t pose_mean, pf_matrix_t pose_cov);

  // Update the filter with new sensor data
  private: void UpdateFilter(amcl_sensor_data_t *data);

#ifdef INCLUDE_RTKGUI
  // Draw the current best pose estimate
  private: void DrawPoseEst();

  // Draw the laser values
  private: void DrawLaserData(amcl_sensor_data_t *data);

  // Draw the sonar values
  private: void DrawSonarData(amcl_sensor_data_t *data);
#endif

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests(void);

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *request, int len);

  // Handle the set pose request
  private: void HandleSetPose(void *client, void *request, int len);

  ///////////////////////////////////////////////////////////////////////////
  // Properties
  ///////////////////////////////////////////////////////////////////////////

  // Odometry device info
  private: CDevice *odom;
  private: int odom_index;

  // Sonar device info
  private: CDevice *sonar;
  private: int sonar_index;

  // Sonar poses relative to robot
  private: int sonar_pose_count;
  private: pf_vector_t sonar_poses[PLAYER_SONAR_MAX_SAMPLES];
  
  // Laser device info
  private: CDevice *laser;
  private: int laser_index;

  // Laser pose relative to robot
  private: pf_vector_t laser_pose;

  // Effective robot radius (used for c-space tests)
  private: double robot_radius;

  // Occupancy map
  private: const char *map_file;
  private: double map_scale;
  private: int map_negate;
  private: map_t *map;

  // Odometry sensor/action model
  private: odometry_t *odom_model;

  // Sonar sensor model
  private: sonar_t *sonar_model;

  // Laser sensor model
  private: laser_t *laser_model;
  private: int laser_max_samples;
  private: double laser_map_err;

  // Odometric pose of last used sensor reading
  private: pf_vector_t odom_pose;

  // Sensor data queue
  private: int q_size, q_start, q_len;
  private: amcl_sensor_data_t *q_data;
  
  // Particle filter
  private: pf_t *pf;
  private: int pf_min_samples, pf_max_samples;
  private: double pf_err, pf_z;

  // Last odometric pose estimates used by filter
  private: pf_vector_t pf_odom_pose;
  private: uint32_t pf_odom_time_sec, pf_odom_time_usec;

  // Initial pose estimate
  private: pf_vector_t pf_init_pose_mean;
  private: pf_matrix_t pf_init_pose_cov;

  // Current particle filter pose estimates
  private: int hyp_count;
  private: amcl_hyp_t hyps[PLAYER_LOCALIZE_MAX_HYPOTHS];

#ifdef INCLUDE_RTKGUI
  // RTK stuff; for testing only
  private: int enable_gui;
  private: rtk_app_t *app;
  private: rtk_canvas_t *canvas;
  private: rtk_fig_t *map_fig;
  private: rtk_fig_t *pf_fig;
  private: rtk_fig_t *robot_fig;
  private: rtk_fig_t *laser_fig;
  private: rtk_fig_t *sonar_fig;
#endif
};


// Initialization function
CDevice* AdaptiveMCL_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_LOCALIZE_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"amcl\" does not support interface \"%s\"\n", interface);
    return (NULL);
  }
  return ((CDevice*) (new AdaptiveMCL(interface, cf, section)));
}


// a driver registration function
void AdaptiveMCL_Register(DriverTable* table)
{
  table->AddDriver("amcl", PLAYER_ALL_MODE, AdaptiveMCL_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
AdaptiveMCL::AdaptiveMCL(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_localize_data_t), 0, 100, 100)
{
  //double size;
  double u[3];
  
  this->odom = NULL;
  this->odom_index = cf->ReadInt(section, "position_index", 0);

  this->sonar = NULL;
  this->sonar_index = cf->ReadInt(section, "sonar_index", -1);

  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", -1);

  // C-space info
  this->robot_radius = cf->ReadLength(section, "robot_radius", 0.20);
  
  // Get the map settings
  this->map_file = cf->ReadFilename(section, "map_file", NULL);
  this->map_scale = cf->ReadLength(section, "map_scale", 0.05);
  this->map_negate = cf->ReadInt(section, "map_negate", 0);
  this->map = NULL;
  
  // Odometry model settings
  this->odom_model = NULL;

  // Laser model settings
  this->laser_model = NULL;
  this->laser_max_samples = cf->ReadInt(section, "laser_max_samples", 6);
  this->laser_map_err = cf->ReadLength(section, "laser_map_err", 0.05);

  // Particle filter settings
  this->pf = NULL;
  this->pf_min_samples = cf->ReadInt(section, "pf_min_samples", 100);
  this->pf_max_samples = cf->ReadInt(section, "pf_max_samples", 10000);

  // Adaptive filter parameters
  this->pf_err = cf->ReadFloat(section, "pf_err", 0.01);
  this->pf_z = cf->ReadFloat(section, "pf_z", 3);
  
  // Initial pose estimate
  this->pf_init_pose_mean = pf_vector_zero();
  this->pf_init_pose_mean.v[0] = cf->ReadTupleLength(section, "init_pose", 0, 0);
  this->pf_init_pose_mean.v[1] = cf->ReadTupleLength(section, "init_pose", 1, 0);
  this->pf_init_pose_mean.v[2] = cf->ReadTupleAngle(section, "init_pose", 2, 0);

  // Initial pose covariance
  u[0] = cf->ReadTupleLength(section, "init_pose_var", 0, 1e3);
  u[1] = cf->ReadTupleLength(section, "init_pose_var", 1, 1e3);
  u[2] = cf->ReadTupleAngle(section, "init_pose_var", 2, 1e2);
  this->pf_init_pose_cov = pf_matrix_zero();
  this->pf_init_pose_cov.m[0][0] = u[0] * u[0];
  this->pf_init_pose_cov.m[1][1] = u[1] * u[1];
  this->pf_init_pose_cov.m[2][2] = u[2] * u[2];

  // Initial hypothesis list
  this->hyp_count = 0;

  // Create the sensor queue
  this->q_len = 0;
  this->q_size = 1000;
  this->q_data = new amcl_sensor_data_t[this->q_size];

#ifdef INCLUDE_RTKGUI
  // Enable debug gui
  this->enable_gui = cf->ReadInt(section, "enable_gui", 0);
#endif
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Destructor
AdaptiveMCL::~AdaptiveMCL(void)
{
  delete[] this->q_data;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int AdaptiveMCL::Setup(void)
{
  amcl_sensor_data_t sdata;
    
  PLAYER_TRACE0("setup");

  // Initialise the underlying position device
  if (this->SetupOdom() != 0)
    return -1;
  
  // Initialise the sonar
  if (this->SetupSonar() != 0)
    return -1;

  // Initialise the laser
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
  PLAYER_MSG1("loading map file [%s]", this->map_file);
  if (map_load_occ(this->map, this->map_file, this->map_negate) != 0)
    return -1;

  // Compute the c-space
  PLAYER_MSG0("computing cspace");
  map_update_cspace(this->map, 2 * this->robot_radius);
  
  // Create the odometry model
  this->odom_model = odometry_alloc(this->map, this->robot_radius);
  if (odometry_init_cspace(this->odom_model))
  {
    PLAYER_ERROR("error generating free space map (this could be a bad map)");
    return -1;
  }

  // Create the sonar model
  this->sonar_model = sonar_alloc(this->map, this->sonar_pose_count, this->sonar_poses);
  
  // Create the laser model
  this->laser_model = laser_alloc(this->map, this->laser_pose);
  this->laser_model->range_cov = this->laser_map_err * this->laser_map_err;

  // Create the particle filter
  assert(this->pf == NULL);
  this->pf = pf_alloc(this->pf_min_samples, this->pf_max_samples);
  this->pf->pop_err = this->pf_err;
  this->pf->pop_z = this->pf_z;

  // Set the initial odometric poses
  this->GetOdomData(&sdata);
  this->odom_pose = sdata.odom_pose;
  this->pf_odom_pose = sdata.odom_pose;
  this->pf_odom_time_sec = sdata.odom_time_sec;
  this->pf_odom_time_usec = sdata.odom_time_usec;

  // Initial hypothesis list
  this->hyp_count = 0;
  
#ifdef INCLUDE_RTKGUI
  // Start the GUI
  if (this->enable_gui)
    this->SetupGUI();
#endif

  // Start the driver thread.
  PLAYER_MSG0("running");
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int AdaptiveMCL::Shutdown(void)
{
  // Stop the driver thread.
  this->StopThread();
    
#ifdef INCLUDE_RTKGUI
  // Stop the GUI
  if (this->enable_gui)
    this->ShutdownGUI();
#endif

  // Delete the particle filter
  pf_free(this->pf);
  this->pf = NULL;

  // Delete the odometry model
  odometry_free(this->odom_model);
  this->odom_model = NULL;

  // Delete the sonar model
  sonar_free(this->sonar_model);
  this->sonar_model = NULL;

  // Delete the laser model
  laser_free(this->laser_model);
  this->laser_model = NULL;
  
  // Delete the map
  map_free(this->map);
  this->map = NULL;
  
  // Stop the laser
  this->ShutdownLaser();

  // Stop the sonar
  this->ShutdownSonar();

  // Stop the odom device
  this->ShutdownOdom();

  PLAYER_TRACE0("shutdown");
  return 0;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Set up the GUI
int AdaptiveMCL::SetupGUI(void)
{
  // Initialize RTK
  rtk_init(NULL, NULL);

  this->app = rtk_app_create();

  this->canvas = rtk_canvas_create(this->app);
  rtk_canvas_title(this->canvas, "AdaptiveMCL");
  rtk_canvas_size(this->canvas, this->map->size_x, this->map->size_y);
  rtk_canvas_scale(this->canvas, this->map->scale, this->map->scale);

  this->map_fig = rtk_fig_create(this->canvas, NULL, -1);
  this->pf_fig = rtk_fig_create(this->canvas, this->map_fig, 5);

  // Draw the map
  map_draw_occ(this->map, this->map_fig);
  //map_draw_cspace(this->map, this->map_fig);

  this->robot_fig = rtk_fig_create(this->canvas, NULL, 9);
  this->laser_fig = rtk_fig_create(this->canvas, this->robot_fig, 10);
  this->sonar_fig = rtk_fig_create(this->canvas, this->robot_fig, 15);

  rtk_fig_movemask(this->robot_fig, RTK_MOVE_TRANS | RTK_MOVE_ROT);

  // Draw the robot
  rtk_fig_color(this->robot_fig, 0.7, 0, 0);
  rtk_fig_rectangle(this->robot_fig, 0, 0, 0, 0.40, 0.20, 0);

  rtk_app_main_init(this->app);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the GUI
int AdaptiveMCL::ShutdownGUI(void)
{
  rtk_fig_destroy(this->sonar_fig);
  rtk_fig_destroy(this->robot_fig);
  rtk_fig_destroy(this->map_fig);
  rtk_fig_destroy(this->pf_fig);
  rtk_canvas_destroy(this->canvas);
  rtk_app_main_term(this->app);
  rtk_app_destroy(this->app);
  
  return 0;
}
  
#endif


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int AdaptiveMCL::SetupOdom(void)
{
  player_device_id_t id;
    
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

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying odom device.
int AdaptiveMCL::ShutdownOdom(void)
{
  this->odom->Unsubscribe(this);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current odometry reading
void AdaptiveMCL::GetOdomData(amcl_sensor_data_t *data)
{
  size_t size;
  player_position_data_t ndata;

  // Get the odom device data.
  size = this->odom->GetData(this, (uint8_t*) &ndata, sizeof(ndata),
                             &data->odom_time_sec, &data->odom_time_usec);

  // Byte swap
  ndata.xpos = ntohl(ndata.xpos);
  ndata.ypos = ntohl(ndata.ypos);
  ndata.yaw = ntohl(ndata.yaw);

  data->odom_pose.v[0] = (double) ((int32_t) ndata.xpos) / 1000.0;
  data->odom_pose.v[1] = (double) ((int32_t) ndata.ypos) / 1000.0;
  data->odom_pose.v[2] = (double) ((int32_t) ndata.yaw) * M_PI / 180;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the sonar
int AdaptiveMCL::SetupSonar(void)
{
  int i;
  uint8_t req;
  uint16_t reptype;
  player_device_id_t id;
  player_sonar_geom_t geom;
  struct timeval tv;

  // If there is no sonar device...
  if (this->sonar_index < 0)
    return 0;
  
  id.code = PLAYER_SONAR_CODE;
  id.index = this->sonar_index;

  this->sonar = deviceTable->GetDevice(id);
  if (!this->sonar)
  {
    PLAYER_ERROR("unable to locate suitable sonar device");
    return -1;
  }
  if (this->sonar->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to sonar device");
    return -1;
  }

  // Get the sonar geometry
  req = PLAYER_SONAR_GET_GEOM_REQ;
  if (this->sonar->Request(&id, this, &req, 1, &reptype, &tv, &geom, sizeof(geom)) < 0)
  {
    PLAYER_ERROR("unable to get sonar geometry");
    return -1;
  }

  this->sonar_pose_count = (int16_t) ntohs(geom.pose_count);
  assert(this->sonar_pose_count < sizeof(this->sonar_poses) / sizeof(this->sonar_poses[0]));
  
  for (i = 0; i < this->sonar_pose_count; i++)
  {
    this->sonar_poses[i].v[0] = ((int16_t) ntohs(geom.poses[i][0])) / 1000.0;
    this->sonar_poses[i].v[1] = ((int16_t) ntohs(geom.poses[i][1])) / 1000.0;
    this->sonar_poses[i].v[2] = ((int16_t) ntohs(geom.poses[i][2])) * M_PI / 180.0;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the sonar
int AdaptiveMCL::ShutdownSonar(void)
{
  // If there is no sonar device...
  if (this->sonar_index < 0)
    return 0;

  this->sonar->Unsubscribe(this);
  this->sonar = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new sonar data
void AdaptiveMCL::GetSonarData(amcl_sensor_data_t *data)
{
  int i;
  size_t size;
  player_sonar_data_t ndata;
  double r; //b, db;

  // If there is no sonar device...
  if (this->sonar_index < 0)
  {
    data->srange_count = 0;
    return;
  }
  
  // Get the sonar device data.
  size = this->sonar->GetData(this, (uint8_t*) &ndata, sizeof(ndata), NULL, NULL);

  data->srange_count = ntohs(ndata.range_count);
  assert(data->srange_count < sizeof(data->sranges) / sizeof(data->sranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < data->srange_count; i++)
  {
    r = ((int16_t) ntohs(ndata.ranges[i])) / 1000.0;
    data->sranges[i] = r;
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int AdaptiveMCL::SetupLaser(void)
{
  uint8_t req;
  uint16_t reptype;
  player_device_id_t id;
  player_laser_geom_t geom;
  struct timeval tv;

  // If there is no laser device...
  if (this->laser_index < 0)
    return 0;

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
  if (this->laser->Request(&id, this, &req, 1, &reptype, &tv, &geom, sizeof(geom)) < 0)
  {
    PLAYER_ERROR("unable to get laser geometry");
    return -1;
  }

  this->laser_pose.v[0] = ((int16_t) ntohl(geom.pose[0])) / 1000.0;
  this->laser_pose.v[1] = ((int16_t) ntohl(geom.pose[1])) / 1000.0;
  this->laser_pose.v[2] = ((int16_t) ntohl(geom.pose[2])) * M_PI / 180.0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int AdaptiveMCL::ShutdownLaser(void)
{
  // If there is no laser device...
  if (this->laser_index < 0)
    return 0;
  
  this->laser->Unsubscribe(this);
  this->laser = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new laser data
void AdaptiveMCL::GetLaserData(amcl_sensor_data_t *data)
{
  int i;
  size_t size;
  player_laser_data_t ndata;
  double r, b, db;
  
  // If there is no laser device...
  if (this->laser_index < 0)
  {
    data->range_count = 0;
    return;
  }

  // Get the laser device data.
  size = this->laser->GetData(this, (uint8_t*) &ndata, sizeof(ndata), NULL, NULL);

  b = ((int16_t) ntohs(ndata.min_angle)) / 100.0 * M_PI / 180.0;
  db = ((int16_t) ntohs(ndata.resolution)) / 100.0 * M_PI / 180.0;

  data->range_count = ntohs(ndata.range_count);
  assert(data->range_count < sizeof(data->ranges) / sizeof(data->ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < data->range_count; i++)
  {
    r = ((int16_t) ntohs(ndata.ranges[i])) / 1000.0;
    data->ranges[i][0] = r;
    data->ranges[i][1] = b;
    b += db;
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current pose.  This function is called by the server thread.
size_t AdaptiveMCL::GetData(void* client, unsigned char* dest, size_t len,
                            uint32_t* time_sec, uint32_t* time_usec)
{
  int i, j, k;
  int datalen;
  player_localize_data_t data;
  pf_vector_t odom_pose, odom_diff;
  pf_vector_t pose;
  pf_matrix_t pose_cov;
  amcl_sensor_data_t sdata;
  amcl_hyp_t *hyp;
  double scale[3];
  
  this->Lock();

  // See if there is new odometry data.  If there is, push it and all
  // the rest of the sensor data onto the sensor queue.
  this->GetOdomData(&sdata);

  // See how far the robot has moved
  odom_pose = sdata.odom_pose;
  odom_diff = pf_vector_coord_sub(odom_pose, this->odom_pose);

  // Make sure we have moved a reasonable distance
  if (fabs(odom_diff.v[0]) > 0.20 ||
      fabs(odom_diff.v[1]) > 0.20 ||
      fabs(odom_diff.v[2]) > M_PI / 6)
  {
    this->odom_pose = sdata.odom_pose;

    // Get the current sonar data; we assume it is new data
    this->GetSonarData(&sdata);

    // Get the current laser data; we assume it is new data
    this->GetLaserData(&sdata);

    // Push the data
    this->Push(&sdata);
  }
  
  // Compute the change in odometric pose
  odom_diff = pf_vector_coord_sub(odom_pose, this->pf_odom_pose);

  // Record the number of pending observations
  data.pending_count = this->q_len;
  data.pending_time_sec = this->pf_odom_time_sec;
  data.pending_time_usec = this->pf_odom_time_usec;
  
  // Encode the hypotheses
  data.hypoth_count = this->hyp_count;
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    // Get the current estimate
    pose = hyp->pf_pose_mean;
    pose_cov = hyp->pf_pose_cov;

    // Translate/rotate the hypotheses to take account of latency in filter
    pose = pf_vector_coord_add(odom_diff, pose);

    // Check for bad values
    if (!pf_vector_finite(pose))
    {
      pf_vector_fprintf(pose, stderr, "%e");
      assert(0);
    }
    if (!pf_matrix_finite(pose_cov))
    {
      pf_matrix_fprintf(pose_cov, stderr, "%e");
      assert(0);
    }

    scale[0] = 1000;
    scale[1] = 1000;
    scale[2] = 3600 * 180 / M_PI;
    
    data.hypoths[i].alpha = (uint32_t) (hyp->weight * 1e6);
        
    data.hypoths[i].mean[0] = (int32_t) (pose.v[0] * scale[0]);
    data.hypoths[i].mean[1] = (int32_t) (pose.v[1] * scale[1]);
    data.hypoths[i].mean[2] = (int32_t) (pose.v[2] * scale[2]);
  
    data.hypoths[i].cov[0][0] = (int64_t) (pose_cov.m[0][0] * scale[0] * scale[0]);
    data.hypoths[i].cov[0][1] = (int64_t) (pose_cov.m[0][1] * scale[1] * scale[1]);
    data.hypoths[i].cov[0][2] = 0;
  
    data.hypoths[i].cov[1][0] = (int64_t) (pose_cov.m[1][0] * scale[0] * scale[0]);
    data.hypoths[i].cov[1][1] = (int64_t) (pose_cov.m[1][1] * scale[1] * scale[1]);
    data.hypoths[i].cov[1][2] = 0;

    data.hypoths[i].cov[2][0] = 0;
    data.hypoths[i].cov[2][1] = 0;
    data.hypoths[i].cov[2][2] = (int64_t) (pose_cov.m[2][2] * scale[2] * scale[2]);
  }

  this->Unlock();
  
  // Compute the length of the data packet
  datalen = sizeof(data) - sizeof(data.hypoths) + data.hypoth_count * sizeof(data.hypoths[0]);

  // Byte-swap
  data.pending_count = htons(data.pending_count);
  data.pending_time_sec = htonl(data.pending_time_sec);
  data.pending_time_usec = htonl(data.pending_time_usec);

  // Byte-swap
  for (i = 0; i < data.hypoth_count; i++)
  {
    for (j = 0; j < 3; j++)
    {
      data.hypoths[i].mean[j] = htonl(data.hypoths[i].mean[j]);
      for (k = 0; k < 3; k++)
        data.hypoths[i].cov[j][k] = htonll(data.hypoths[i].cov[j][k]);
    }
    data.hypoths[i].alpha = htonl(data.hypoths[i].alpha);
  }
  data.hypoth_count = htonl(data.hypoth_count);

  // Copy data to server
  assert(len >= datalen);
  assert(dest != NULL);
  memcpy(dest, &data, datalen);

  // Set the timestamp
  if (time_sec)
    *time_sec = sdata.odom_time_sec;
  if (time_usec)
    *time_usec = sdata.odom_time_usec;

  return datalen;
}


////////////////////////////////////////////////////////////////////////////////
// Process configuration requests
int AdaptiveMCL::PutConfig(player_device_id_t* device, void* client,
                           void* data, size_t len)
{
  // Discard bogus empty packets
  if (len < 1)
  {
    PLAYER_WARN("got zero length configuration request; ignoring");
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return 0;
  }

  // Process some of the requests immediately
  switch (((char*) data)[0])
  {
    case PLAYER_LOCALIZE_GET_MAP_INFO_REQ:
      HandleGetMapInfo(client, data, len);
      return 0;
    case PLAYER_LOCALIZE_GET_MAP_DATA_REQ:
      HandleGetMapData(client, data, len);
      return 0;
  }

  // Let the device thread get the rest
  return CDevice::PutConfig(device, client, data, len);
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
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  //PLAYER_TRACE4("%d %d %f %d\n", this->map->size_x, this->map->size_y,
  //              this->map->scale, ntohl(info.scale));
  
  info.scale = htonl((int32_t) (1000.0 / this->map->scale + 0.5));
  info.width = htonl((int32_t) (this->map->size_x));
  info.height = htonl((int32_t) (this->map->size_y));

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
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
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

  //PLAYER_TRACE4("%d %d %d %d\n", oi, oj, si, sj);

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


////////////////////////////////////////////////////////////////////////////////
// Push data onto the filter queue.
void AdaptiveMCL::Push(amcl_sensor_data_t *data)
{
  int i;
  
  if (this->q_len >= this->q_size)
  {
    PLAYER_ERROR("queue overflow");
    return;
  }

  i = (this->q_start + this->q_len++) % this->q_size;
  this->q_data[i] = *data;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Pop data from the filter queue
int AdaptiveMCL::Pop(amcl_sensor_data_t *data)
{
  int i;
  
  if (this->q_len == 0)
    return 0;

  //PLAYER_TRACE1("q len %d\n", this->q_len);
  
  i = this->q_start++ % this->q_size;
  this->q_len--;
  *data = this->q_data[i];
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void AdaptiveMCL::Main(void) 
{  
  struct timespec sleeptime;
  amcl_sensor_data_t data;
  int init = 0;
  
  // WARNING: this only works for Linux
  // Run at a lower priority
  nice(10);

  // Initialize the filter
  this->InitFilter(this->pf_init_pose_mean, this->pf_init_pose_cov);
  
  while (true)
  {
#ifdef INCLUDE_RTKGUI
    if (this->enable_gui)
    {
      rtk_canvas_render(this->canvas);
      rtk_app_main_loop(this->app);
    }
#endif

    // Sleep for 1ms (will actually take longer than this).
    sleeptime.tv_sec = 0;
    sleeptime.tv_nsec = 1000000L;
    nanosleep(&sleeptime, NULL);

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any pending requests.
    this->HandleRequests();

    // Process any queued data
    if (this->Pop(&data))
    {
      init = 1; // TESTING
      this->UpdateFilter(&data);
    }

#ifdef INCLUDE_RTKGUI
    // TESTING
    if (this->enable_gui)
    {
      if (init)
      {
        DrawLaserData(&data);
        DrawSonarData(&data);
      }
    }
#endif
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize the filter
void AdaptiveMCL::InitFilter(pf_vector_t pose_mean, pf_matrix_t pose_cov)
{
  int i;
  double weight;
  amcl_hyp_t *hyp;
  
  // Initialize the odometric model
  odometry_init_init(this->odom_model, pose_mean, pose_cov);
  
  // Draw samples from the odometric distribution
  pf_init(this->pf, (pf_init_model_fn_t) odometry_init_model, this->odom_model);
  
  odometry_init_term(this->odom_model);

  this->Lock();

  // Get the hypotheses
  this->hyp_count = 0;
  for (i = 0; i < sizeof(this->hyps) / sizeof(this->hyps[0]); i++)
  {
    if (!pf_get_cluster_stats(this->pf, i, &weight, &pose_mean, &pose_cov))
      break;

    hyp = this->hyps + this->hyp_count++;
    hyp->weight = weight;
    hyp->pf_pose_mean = pose_mean;
    hyp->pf_pose_cov = pose_cov;
  }

  this->Unlock();

#ifdef INCLUDE_RTKGUI
  // Draw the samples
  if (this->enable_gui)
  {
    DrawPoseEst();
    
    rtk_fig_clear(this->pf_fig);
    rtk_fig_color(this->pf_fig, 0, 0, 1);
    pf_draw_samples(this->pf, this->pf_fig, 1000);
    //rtk_fig_color(this->pf_fig, 0, 1, 0);
    //pf_draw_stats(this->pf, this->pf_fig);
    //pf_draw_hist(this->pf, this->pf_fig);
  }
#endif

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the filter with new sensor data
void AdaptiveMCL::UpdateFilter(amcl_sensor_data_t *data)
{
  int i, step;
  double weight;
  pf_vector_t pose_mean;
  pf_matrix_t pose_cov;
  amcl_hyp_t *hyp;

  // Update the odometry sensor model with the latest odometry measurements
  odometry_action_init(this->odom_model, this->pf_odom_pose, data->odom_pose);
  odometry_sensor_init(this->odom_model);

  // Apply the odometry action model
  pf_update_action(this->pf, (pf_action_model_fn_t) odometry_action_model, this->odom_model);

  // Apply the odometry sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) odometry_sensor_model, this->odom_model);

  odometry_sensor_term(this->odom_model);
  odometry_action_term(this->odom_model);

  // Update the sonar sensor model with the latest sonar measurements
  sonar_clear_ranges(this->sonar_model);
  for (i = 0; i < data->srange_count; i++)
    sonar_add_range(this->sonar_model, data->sranges[i]);

  // Apply the sonar sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) sonar_sensor_model, this->sonar_model);  
  
  // Update the laser sensor model with the latest laser measurements
  if (this->laser_max_samples >= 2)
  {
    laser_clear_ranges(this->laser_model);
    
    step = (data->range_count - 1) / (this->laser_max_samples - 1);
    for (i = 0; i < data->range_count; i += step)
      laser_add_range(this->laser_model, data->ranges[i][0], data->ranges[i][1]);

    // Apply the laser sensor model
    pf_update_sensor(this->pf, (pf_sensor_model_fn_t) laser_sensor_model, this->laser_model);
  }

  // Resample
  pf_update_resample(this->pf);
  
  this->Lock();

  this->pf_odom_pose = data->odom_pose;
  this->pf_odom_time_sec = data->odom_time_sec;
  this->pf_odom_time_usec = data->odom_time_usec;

  // Get the hypotheses
  this->hyp_count = 0;
  for (i = 0; i < sizeof(this->hyps) / sizeof(this->hyps[0]); i++)
  {
    if (!pf_get_cluster_stats(this->pf, i, &weight, &pose_mean, &pose_cov))
      break;

    hyp = this->hyps + this->hyp_count++;
    hyp->weight = weight;
    hyp->pf_pose_mean = pose_mean;
    hyp->pf_pose_cov = pose_cov;
  }

  this->Unlock();


#ifdef INCLUDE_RTKGUI
  // Draw the samples
  if (this->enable_gui)
  {
    DrawPoseEst();
    DrawLaserData(data);
    DrawSonarData(data);
    
    rtk_fig_clear(this->pf_fig);
    rtk_fig_color(this->pf_fig, 0, 0, 1);
    pf_draw_samples(this->pf, this->pf_fig, 1000);
    //rtk_fig_color(this->pf_fig, 0, 1, 0);
    //pf_draw_stats(this->pf, this->pf_fig);
    //pf_draw_hist(this->pf, this->pf_fig);
  }
#endif

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int AdaptiveMCL::HandleRequests(void)
{
  int len;
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      case PLAYER_LOCALIZE_SET_POSE_REQ:
        HandleSetPose(client, request, len);
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
// Handle the set pose request
void AdaptiveMCL::HandleSetPose(void *client, void *request, int len)
{
  size_t reqlen;
  player_localize_set_pose_t req;
  pf_vector_t pose;
  pf_matrix_t cov;

  // Expected length of request
  reqlen = sizeof(req);
  
  // check if the config request is valid
  if (len != reqlen)
  {
    PLAYER_ERROR2("config request len is invalid (%d != %d)", len, reqlen);
    if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
      PLAYER_ERROR("PutReply() failed");
    return;
  }

  req = *((player_localize_set_pose_t*) request);

  pose.v[0] = ((int32_t) ntohl(req.mean[0])) / 1000.0;
  pose.v[1] = ((int32_t) ntohl(req.mean[1])) / 1000.0;
  pose.v[2] = ((int32_t) ntohl(req.mean[2])) / 3600.0 * M_PI / 180;
    
  cov = pf_matrix_zero();
  cov.m[0][0] = ((int64_t) ntohll(req.cov[0][0])) / 1e6;
  cov.m[0][1] = ((int64_t) ntohll(req.cov[0][1])) / 1e6;
  cov.m[1][0] = ((int64_t) ntohll(req.cov[1][0])) / 1e6;
  cov.m[1][1] = ((int64_t) ntohll(req.cov[1][1])) / 1e6;
  cov.m[2][2] = ((int64_t) ntohll(req.cov[2][2])) / (3600.0 * 3600.0) * (M_PI / 180 * M_PI / 180);

  // Initialize the filter
  this->InitFilter(pose, cov);

  // Give them an ack
  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Draw the current best pose estimate
void AdaptiveMCL::DrawPoseEst()
{
  int i;
  double max_weight;
  amcl_hyp_t *hyp;
  
  this->Lock();

  max_weight = 0;
  for (i = 0; i < this->hyp_count; i++)
  {
    hyp = this->hyps + i;

    if (hyp->weight > max_weight)
    {
      max_weight = hyp->weight;
      rtk_fig_origin(this->robot_fig, hyp->pf_pose_mean.v[0],
                     hyp->pf_pose_mean.v[1], hyp->pf_pose_mean.v[2]);
    }
  }
  
  this->Unlock();
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the laser values
void AdaptiveMCL::DrawLaserData(amcl_sensor_data_t *data)
{
  int i, step;
  pf_vector_t pose;
  double r, b, m, ax, ay, bx, by;
  
  rtk_fig_clear(this->laser_fig);

  // Draw the complete scan
  rtk_fig_color_rgb32(this->laser_fig, 0x8080FF);
  for (i = 0; i < data->range_count; i++)
  {
    r = data->ranges[i][0];
    b = data->ranges[i][1];

    ax = 0;
    ay = 0;
    bx = ax + r * cos(b);
    by = ay + r * sin(b);

    rtk_fig_line(this->laser_fig, ax, ay, bx, by);
  }
    
  // Draw the significant part of the scan
  if (this->laser_max_samples >= 2)
  {
    // Get the robot figure pose
    rtk_fig_get_origin(this->robot_fig, pose.v + 0, pose.v + 1, pose.v + 2);
        
    step = (data->range_count - 1) / (this->laser_max_samples - 1);
    for (i = 0; i < data->range_count; i += step)
    {
      r = data->ranges[i][0];
      b = data->ranges[i][1];
      m = map_calc_range(this->map, pose.v[0], pose.v[1], pose.v[2] + b, 8.0);

      ax = 0;
      ay = 0;

      bx = ax + r * cos(b);
      by = ay + r * sin(b);
      rtk_fig_color_rgb32(this->laser_fig, 0xFF0000);
      rtk_fig_line(this->laser_fig, ax, ay, bx, by);

      bx = ax + m * cos(b);
      by = ay + m * sin(b);
      rtk_fig_color_rgb32(this->laser_fig, 0x00FF00);
      rtk_fig_line(this->laser_fig, ax, ay, bx, by);
    }

    // TESTING
    laser_clear_ranges(this->laser_model);
    for (i = 0; i < data->range_count; i += step)
      laser_add_range(this->laser_model, data->ranges[i][0], data->ranges[i][1]);
    //printf("prob = %f\n", laser_sensor_model(this->laser_model, pose));
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw the sonar values
void AdaptiveMCL::DrawSonarData(amcl_sensor_data_t *data)
{
  int i;
  double r, b, ax, ay, bx, by;
  
  rtk_fig_clear(this->sonar_fig);
  rtk_fig_color_rgb32(this->sonar_fig, 0xC0C080);
  
  for (i = 0; i < data->srange_count; i++)
  {
    r = data->sranges[i];
    b = this->sonar_poses[i].v[2];

    ax = this->sonar_poses[i].v[0];
    ay = this->sonar_poses[i].v[1];

    bx = ax + r * cos(b);
    by = ay + r * sin(b);
    
    rtk_fig_line(this->sonar_fig, ax, ay, bx, by);
  }
  return;
}

#endif


