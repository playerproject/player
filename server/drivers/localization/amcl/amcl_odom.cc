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
#include "devicetable.h"
#include "amcl_odom.h"


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLOdom::AMCLOdom()
{
  this->device = NULL;
  this->model = NULL;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load settings
int AMCLOdom::Load(ConfigFile* cf, int section)
{
  this->odom_index = cf->ReadInt(section, "odom_index", 0);

  // Create the odometry model
  this->model = odometry_alloc();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLOdom::Unload(void)
{
  odometry_free(this->model);
  this->model = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int AMCLOdom::Setup(void)
{
  player_device_id_t id;
  
  // Subscribe to the odometry device
  id.code = PLAYER_POSITION_CODE;
  id.index = this->odom_index;
  this->device = deviceTable->GetDevice(id);
  if (!this->device)
  {
    PLAYER_ERROR("unable to locate suitable position device");
    return -1;
  }
  if (this->device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to position device");
    return -1;
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the underlying odom device.
int AMCLOdom::Shutdown(void)
{
  // Unsubscribe from device
  this->device->Unsubscribe(this);
  this->device = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Get the current odometry reading
bool AMCLOdom::GetData(void)
{
  size_t size;
  uint32_t tsec, tusec;
  player_position_data_t data;

  // Get the odom device data.
  size = this->device->GetData(this, (uint8_t*) &data, sizeof(data), &tsec, &tusec);
  if (size == 0)
    return false;

  if (tsec == this->tsec && tusec == this->tusec)
    return false;

  this->tsec = tsec;
  this->tusec = tusec;
  
  // Byte swap
  data.xpos = ntohl(data.xpos);
  data.ypos = ntohl(data.ypos);
  data.yaw = ntohl(data.yaw);

  this->tsec = tsec;
  this->tusec = tusec;
  this->pose.v[0] = (double) ((int32_t) data.xpos) / 1000.0;
  this->pose.v[1] = (double) ((int32_t) data.ypos) / 1000.0;
  this->pose.v[2] = (double) ((int32_t) data.yaw) * M_PI / 180;

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize the action model
bool AMCLOdom::InitAction(pf_t *pf, uint32_t *tsec, uint32_t *tusec)
{
  this->tsec = 0;
  this->tusec = 0;

  if (!this->GetData())
    return false;

  this->last_pose = this->pose;
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the action model
bool AMCLOdom::UpdateAction(pf_t *pf, uint32_t *tsec, uint32_t *tusec)
{
  pf_vector_t diff;
  double valid_dist, valid_yaw;

  valid_dist = 1.0;
  valid_yaw = M_PI;

  // Check for new data
  if (!this->GetData())
    return false;
  
  // See how far the robot has moved
  diff = pf_vector_coord_sub(this->pose, this->last_pose);

  /* TESTING
  // Check for jumps in odometry.  This shouldnt happen, but the P2OS
  // driver doesnt always produce correct odometry on the P2AT.
  if (fabs(diff.v[0]) > valid_dist || fabs(diff.v[1]) > valid_dist || fabs(diff.v[2]) > valid_yaw)
  {
    PLAYER_WARN3("invalid odometry change [%f %f %f]; ignoring",
                 diff.v[0], diff.v[1], diff.v[2]);
    *tsec = this->tsec;
    *tusec = this->tusec;
    this->last_pose = this->pose;
    return false;
  }
  */
  
  // Make sure we have moved a reasonable distance
  if (fabs(diff.v[0]) < 0.20 && fabs(diff.v[1]) < 0.20 && fabs(diff.v[2]) < M_PI / 6)
    return false;

  printf("odom: %f %f %f : %f %f %f\n",
         this->last_pose.v[0], this->last_pose.v[1], this->last_pose.v[2],
         this->pose.v[0], this->pose.v[1], this->pose.v[2]);
  
  // Apply the odometry action model
  odometry_action_init(this->model, this->last_pose, this->pose);
  pf_update_action(pf, (pf_action_model_fn_t) odometry_action_model, this->model);
  odometry_action_term(this->model);

  *tsec = this->tsec;
  *tusec = this->tusec;
  this->last_pose = this->pose;
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize the filter
bool AMCLOdom::InitSensor(pf_t *pf, pf_vector_t mean, pf_matrix_t cov)
{    
  // Initialize the odometric model
  odometry_init_init(this->model, mean, cov);
  
  // Draw samples from the odometric distribution
  pf_init(pf, (pf_init_model_fn_t) odometry_init_model, this->model);
  
  odometry_init_term(this->model);

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the sensor model
bool AMCLOdom::UpdateSensor(pf_t *pf)
{
  // We dont have a sensor model; do nothing
  return false;
}


#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLOdom::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLOdom::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Draw sensor data
void AMCLOdom::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  return;
}

#endif
