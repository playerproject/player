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
#include "amcl_sensor.h"


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

  // Check for updated sensor data
  public: virtual void Update(void);

  /* REMOVE
  // Process configuration requests
  public: virtual int PutConfig(player_device_id_t* device, void* client, 
                                void* data, size_t len);

  // Handle map info request
  private: void HandleGetMapInfo(void *client, void *request, int len);

  // Handle map data request
  private: void HandleGetMapData(void *client, void *request, int len);
  */

  ///////////////////////////////////////////////////////////////////////////
  // Middle methods: these methods facilitate communication between the top
  // and bottom halfs.
  ///////////////////////////////////////////////////////////////////////////

  // Push data onto the queue
  private: void Push(AMCLSensorData *data);

  // Take a peek at the queue
  private: AMCLSensorData *Peek(void);

  // Pop data from the queue
  private: AMCLSensorData *Pop(void);

  ///////////////////////////////////////////////////////////////////////////
  // Bottom half methods; these methods run in the device thread
  ///////////////////////////////////////////////////////////////////////////
  
  // Main function for device thread.
  private: virtual void Main(void);

  // Device thread finalization
  private: virtual void MainQuit();

  // Initialize the filter
  private: void InitFilter(void);

  // Update/initialize the filter with new sensor data
  private: bool UpdateFilter();

  // Put new localization data
  private: void PutDataLocalize(uint32_t tsec, uint32_t tusec);

  // Put new position data
  private: void PutDataPosition(uint32_t tsec, uint32_t tusec, pf_vector_t delta);

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests(void);

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *request, int len);

  // Handle the set pose request
  private: void HandleSetPose(void *client, void *request, int len);

#ifdef INCLUDE_RTKGUI
  // Set up the GUI
  private: int SetupGUI(void);

  // Shut down the GUI
  private: int ShutdownGUI(void);

  // Update the GUI
  private: void UpdateGUI(void);

  // Draw the current best pose estimate
  private: void DrawPoseEst();
#endif

  ///////////////////////////////////////////////////////////////////////////
  // Properties
  ///////////////////////////////////////////////////////////////////////////

  // Which interface we are using
  private: char *interface;
  
  // List of all sensors
  private: int sensor_count;
  private: AMCLSensor *sensors[16];

  // Index of sensor providing initialization model
  private: int init_sensor;

  // Index of sensor providing action model
  private: int action_sensor;

  // Particle filter
  private: pf_t *pf;
  private: int pf_min_samples, pf_max_samples;
  private: double pf_err, pf_z;

  // Sensor data queue
  private: int q_size, q_start, q_len;
  private: AMCLSensorData **q_data;
  
  // Current particle filter pose estimates
  private: int hyp_count;
  private: amcl_hyp_t hyps[PLAYER_LOCALIZE_MAX_HYPOTHS];

  // Has the filter been initialized?
  private: bool pf_init;

  // Initial pose estimate; used for filter initialization
  private: pf_vector_t pf_init_pose_mean;
  private: pf_matrix_t pf_init_pose_cov;

  // Last odometric pose value used to update filter
  private: pf_vector_t pf_odom_pose;

  // Minimum update distances
  private: double min_dr, min_da;

#ifdef INCLUDE_RTKGUI
  // RTK stuff; for testing only
  private: int enable_gui;
  private: rtk_app_t *app;
  private: rtk_canvas_t *canvas;
  private: rtk_fig_t *map_fig;
  private: rtk_fig_t *pf_fig;
  private: rtk_fig_t *robot_fig;
#endif

#ifdef INCLUDE_OUTFILE
  private: FILE *outfile;
#endif
};

#endif
