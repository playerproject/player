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
// Desc: AMCL odometry routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "amcl.h"


////////////////////////////////////////////////////////////////////////////////
// Load odometry settings
int AdaptiveMCL::LoadOdom(ConfigFile* cf, int section)
{
  this->odom = NULL;
  this->odom_index = cf->ReadInt(section, "odom_index", 0);
  this->robot_radius = cf->ReadLength(section, "robot_radius", 0.20);
  this->odom_model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int AdaptiveMCL::SetupOdom(void)
{
  player_device_id_t id;

  if (this->odom_index < 0)
    return 0;
  
  // Subscribe to the odometry device
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
  
  // Create the odometry model
  this->odom_model = odometry_alloc(this->map, this->robot_radius);
  if (odometry_init_cspace(this->odom_model))
  {
    PLAYER_ERROR("error generating free space map (this could be a bad map)");
    return -1;
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying odom device.
int AdaptiveMCL::ShutdownOdom(void)
{
  if (this->odom_index < 0)
    return 0;

  this->odom->Unsubscribe(this);
  this->laser = NULL;
  
  // Delete the odometry model
  odometry_free(this->odom_model);
  this->odom_model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current odometry reading
void AdaptiveMCL::GetOdomData(amcl_sensor_data_t *data)
{
  size_t size;
  player_position_data_t ndata;

  if (this->odom_index < 0)
    return;

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
// Initialize from the odom sensor model
bool AdaptiveMCL::InitOdomModel(amcl_sensor_data_t *data)
{
  pf_vector_t pose_mean;
  pf_matrix_t pose_cov;

  if (this->odom_index < 0)
    return false;

  if (this->map == NULL)
  {
    PLAYER_WARN("odometric init failed (no map)");
    return false;
  }
  
  pose_mean = this->pf_init_pose_mean;
  pose_cov =  this->pf_init_pose_cov,
    
  // Initialize the odometric model
  odometry_init_init(this->odom_model, pose_mean, pose_cov);
  
  // Draw samples from the odometric distribution
  ::pf_init(this->pf, (pf_init_model_fn_t) odometry_init_model, this->odom_model);
  
  odometry_init_term(this->odom_model);

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the odom sensor model
bool AdaptiveMCL::UpdateOdomModel(amcl_sensor_data_t *data)
{
  pf_vector_t odom_diff;
  
  if (this->odom_index < 0)
    return false;

  // See how far the robot has moved
  odom_diff = pf_vector_coord_sub(data->odom_pose, this->pf_odom_pose);

  // Make sure we have moved a reasonable distance
  if (fabs(odom_diff.v[0]) < 0.20 &&
      fabs(odom_diff.v[1]) < 0.20 &&
      fabs(odom_diff.v[2]) < M_PI / 6)
    return false;

  printf("odom: %f %f %f : %f %f %f\n",
         this->pf_odom_pose.v[0], this->pf_odom_pose.v[1], this->pf_odom_pose.v[2],
         data->odom_pose.v[0], data->odom_pose.v[1], data->odom_pose.v[2]);
  
  // Update the odometry sensor model with the latest odometry measurements
  odometry_action_init(this->odom_model, this->pf_odom_pose, data->odom_pose);
  odometry_sensor_init(this->odom_model);

  // Apply the odometry action model
  pf_update_action(this->pf, (pf_action_model_fn_t) odometry_action_model, this->odom_model);

  // Apply the odometry sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) odometry_sensor_model, this->odom_model);

  odometry_sensor_term(this->odom_model);
  odometry_action_term(this->odom_model);

  this->Lock();
  this->pf_odom_pose = data->odom_pose;
  this->pf_odom_time_sec = data->odom_time_sec;
  this->pf_odom_time_usec = data->odom_time_usec;
  this->Unlock();
  
  return true;
}
