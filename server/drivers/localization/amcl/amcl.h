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
// Requires: position (odometry), laser, sonar, gps
// Provides: localization
//
///////////////////////////////////////////////////////////////////////////

#ifndef AMCL_H
#define AMCL_H

#ifdef INCLUDE_RTKGUI
#include "rtk.h"
#endif

#include "player.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

#include "pf/pf.h"
#include "map/map.h"
#include "models/odometry.h"
#include "models/sonar.h"
#include "models/laser.h"
#include "models/wifi.h"
#include "models/gps.h"
#include "models/imu.h"


// Combined sensor data packet
typedef struct
{
  // Odometry data
  uint32_t odom_time_sec, odom_time_usec;
  pf_vector_t odom_pose;

  // Sonar ranges
  int sonar_range_count;
  double sonar_ranges[PLAYER_SONAR_MAX_SAMPLES];

  // Laser ranges
  int laser_range_count;
  double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2];

  // Wifi
  int wifi_level_count;
  int wifi_levels[PLAYER_WIFI_MAX_LINKS];

  // GPS
  double gps_time;
  double gps_utm_e, gps_utm_n;
  double gps_err_horz;

  // IMU
  double imu_time;
  double imu_utm_head;
  
} amcl_sensor_data_t;


// Wifi beacon data
typedef struct
{
  // Beacon hostname/ip address
  const char *hostname;

  // Wifi map filename
  const char *filename;
  
} amcl_wifi_beacon_t;


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
  // Top half methods; these methods run in the server thread (except for
  // the sensor Init and Update functions, which run in the driver thread).
  ///////////////////////////////////////////////////////////////////////////

  // Constructor
  public: AdaptiveMCL(char* interface, ConfigFile* cf, int section);

  // Destructor
  public: virtual ~AdaptiveMCL(void);

  // Setup/shutdown routines.
  public: virtual int Setup(void);
  public: virtual int Shutdown(void);

  // Set up the odometry device
  private: int LoadOdom(ConfigFile* cf, int section);  
  private: int SetupOdom(void);
  private: int ShutdownOdom(void);
  private: void GetOdomData(amcl_sensor_data_t *data);
  
  // Set up the sonar device
  private: int LoadSonar(ConfigFile* cf, int section);
  private: int SetupSonar(void);
  private: int ShutdownSonar(void);
  private: void GetSonarData(amcl_sensor_data_t *data);
  
  // Set up the laser device
  private: int LoadLaser(ConfigFile* cf, int section);
  private: int SetupLaser(void);
  private: int ShutdownLaser(void);
  private: void GetLaserData(amcl_sensor_data_t *data);
  
  // Set up the wifi device
  private: int LoadWifi(ConfigFile* cf, int section);
  private: int SetupWifi(void);
  private: int ShutdownWifi(void);
  private: void GetWifiData(amcl_sensor_data_t *data);

  // Set up the gps device
  private: int LoadGps(ConfigFile* cf, int section);
  private: int SetupGps(void);
  private: int ShutdownGps(void);
  private: void GetGpsData(amcl_sensor_data_t *data);

  // Set up the imu device
  private: int LoadImu(ConfigFile* cf, int section);
  private: int SetupImu(void);
  private: int ShutdownImu(void);
  private: void GetImuData(amcl_sensor_data_t *data);

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

  // Device thread finalization
  private: virtual void MainQuit();

  // Initialize the filter
  private: void InitFilter(pf_vector_t pose_mean, pf_matrix_t pose_cov);

  // Update the filter with new sensor data
  private: bool UpdateFilter(amcl_sensor_data_t *data);

  // Apply sensor models
  private: bool InitOdomModel(amcl_sensor_data_t *data);
  private: bool UpdateOdomModel(amcl_sensor_data_t *data);
  private: bool UpdateSonarModel(amcl_sensor_data_t *data);
  private: bool UpdateLaserModel(amcl_sensor_data_t *data);
  private: bool UpdateWifiModel(amcl_sensor_data_t *data);
  private: bool InitGpsModel(amcl_sensor_data_t *data);
  private: bool UpdateGpsModel(amcl_sensor_data_t *data);
  private: bool UpdateImuModel(amcl_sensor_data_t *data);

#ifdef INCLUDE_RTKGUI
  // Set up the GUI
  private: int SetupGUI(void);

  // Shut down the GUI
  private: int ShutdownGUI(void);

  // Draw the current best pose estimate
  private: void DrawPoseEst();

  // Draw the sensor values
  private: void DrawSonarData(amcl_sensor_data_t *data);
  private: void DrawLaserData(amcl_sensor_data_t *data);
  private: void DrawWifiData(amcl_sensor_data_t *data);
  private: void DrawGpsData(amcl_sensor_data_t *data);
  private: void DrawImuData(amcl_sensor_data_t *data);
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

  // Occupancy map info
  private: const char *occ_filename;
  private: int occ_map_negate;

  // Effective robot radius (used for c-space tests)
  private: double robot_radius;

  // The global map
  private: map_t *map;
  private: double map_scale;

  // Particle filter
  private: pf_t *pf;
  private: int pf_min_samples, pf_max_samples;
  private: double pf_err, pf_z;

  // Has the initial data been pushed?
  private: bool push_init;

  // Has the filter been initialized?
  private: bool pf_init;

  // Which sensor should be used to initialized
  private: bool pf_init_odom;
  private: bool pf_init_gps;
  
  // Initial pose estimate; these are used for odometry-based
  // initialization
  private: pf_vector_t pf_init_pose_mean;
  private: pf_matrix_t pf_init_pose_cov;

  // Last odometric pose estimates used by filter
  private: pf_vector_t pf_odom_pose;
  private: uint32_t pf_odom_time_sec, pf_odom_time_usec;
  
  // Odometric pose of last used sensor reading
  private: pf_vector_t odom_pose;

  // Sensor data queue
  private: int q_size, q_start, q_len;
  private: amcl_sensor_data_t *q_data;
  
  // Current particle filter pose estimates
  private: int hyp_count;
  private: amcl_hyp_t hyps[PLAYER_LOCALIZE_MAX_HYPOTHS];

  // Odometry device info
  private: CDevice *odom;
  private: int odom_index;

  // Odometry sensor/action model
  private: odometry_t *odom_model;

  // Sonar device info
  private: CDevice *sonar;
  private: int sonar_index;

  // Sonar poses relative to robot
  private: int sonar_pose_count;
  private: pf_vector_t sonar_poses[PLAYER_SONAR_MAX_SAMPLES];

  // Sonar sensor model
  private: sonar_t *sonar_model;

  // Laser device info
  private: CDevice *laser;
  private: int laser_index;

  // Laser pose relative to robot
  private: pf_vector_t laser_pose;

  // Laser sensor model
  private: laser_t *laser_model;
  private: int laser_max_samples;
  private: double laser_map_err;

  // Wifi device info
  private: CDevice *wifi;
  private: int wifi_index;

  // List of possible wifi beacons
  private: int wifi_beacon_count;
  private: amcl_wifi_beacon_t wifi_beacons[PLAYER_WIFI_MAX_LINKS];

  // WiFi sensor model
  private: wifi_t *wifi_model;

  // GPS device info
  private: CDevice *gps;
  private: int gps_index;
  private: double gps_time;

  // GPS sensor model
  private: gps_model_t *gps_model;

  // IMU device info
  private: CDevice *imu;
  private: int imu_index;

  // IMU sensor model
  private: double imu_mag_dev;
  private: imu_model_t *imu_model;

#ifdef INCLUDE_RTKGUI
  // RTK stuff; for testing only
  private: int enable_gui;
  private: rtk_app_t *app;
  private: rtk_canvas_t *canvas;
  private: rtk_fig_t *map_fig;
  private: rtk_fig_t *pf_fig;
  private: rtk_fig_t *robot_fig;
  private: rtk_fig_t *sonar_fig;
  private: rtk_fig_t *laser_fig;
  private: rtk_fig_t *wifi_fig;
  private: rtk_fig_t *gps_fig;
  private: rtk_fig_t *imu_fig;
  // REMOVE private: double origin_ox, origin_oy;
#endif

#ifdef INCLUDE_OUTFILE
  private: FILE *outfile;
#endif
};

#endif
