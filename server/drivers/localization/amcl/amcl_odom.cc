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

extern int global_playerport;


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLOdom::AMCLOdom()
{
  this->device = NULL;
  this->action_pdf = NULL;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load settings
int AMCLOdom::Load(ConfigFile* cf, int section)
{
  this->odom_index = cf->ReadInt(section, "odom_index", 0);

  this->tsec = 0;
  this->tusec = 0;
  this->last_pose = pf_vector_zero();

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLOdom::Unload(void)
{  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the underlying odom device.
int AMCLOdom::Setup(void)
{
  player_device_id_t id;
  
  // Subscribe to the odometry device
  id.port = global_playerport;
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
AMCLSensorData *AMCLOdom::GetData(void)
{
  size_t size;
  uint32_t tsec, tusec;
  pf_vector_t pose, delta;
  player_position_data_t data;
  AMCLOdomData *ndata;

  // Get the odom device data.
  size = this->device->GetData(this, (uint8_t*) &data, sizeof(data), &tsec, &tusec);
  if (size == 0)
    return false;

  // See if this is a new reading
  if (tsec == this->tsec && tusec == this->tusec)
    return NULL;

  // Byte swap
  data.xpos = ntohl(data.xpos);
  data.ypos = ntohl(data.ypos);
  data.yaw = ntohl(data.yaw);

  // Compute new robot pose
  pose.v[0] = (double) ((int32_t) data.xpos) / 1000.0;
  pose.v[1] = (double) ((int32_t) data.ypos) / 1000.0;
  pose.v[2] = (double) ((int32_t) data.yaw) * M_PI / 180;

  // If this is not the first pass...
  if (this->tsec == 0 && this->tusec == 0)
  {
    ndata = new AMCLOdomData;

    ndata->sensor = this;
    ndata->tsec = tsec;
    ndata->tusec = tusec;

    ndata->pose = pose;
    ndata->delta = pf_vector_zero();

    this->last_pose = pose;
    this->tsec = tsec;
    this->tusec = tusec;
    
    return ndata;
  }
  else
  {
    this->tsec = tsec;
    this->tusec = tusec;
  }

  // Compute change in pose
  delta = pf_vector_coord_sub(pose, this->last_pose);

  // HACK: fix
  double min_dr = 0.20;
  double min_da = M_PI / 6;

  // Make sure we have moved a reasonable distance
  if (fabs(delta.v[0]) > min_dr || fabs(delta.v[1]) > min_dr || fabs(delta.v[2]) > min_da)
  {
    ndata = new AMCLOdomData;

    ndata->sensor = this;
    ndata->tsec = tsec;
    ndata->tusec = tusec;
    ndata->pose = pose;
    ndata->delta = delta;

    this->last_pose = pose;

    return ndata;
  }

  return NULL;
  
  /* TESTING
     double valid_dist, valid_yaw;
     valid_dist = 1.0;
     valid_yaw = M_PI;

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
}


////////////////////////////////////////////////////////////////////////////////
// Apply the action model
bool AMCLOdom::UpdateAction(pf_t *pf, AMCLSensorData *data)
{
  AMCLOdomData *ndata;
  pf_vector_t x;
  pf_matrix_t cx;
  double ux, uy, ua;
  ndata = (AMCLOdomData*) data;

  /*
  printf("odom: %f %f %f : %f %f %f\n",
         ndata->pose.v[0], ndata->pose.v[1], ndata->pose.v[2],
         ndata->delta.v[0], ndata->delta.v[1], ndata->delta.v[2]);
  */
         
  // See how far the robot has moved
  x = ndata->delta;

  // HACK - FIX
  ux = 0.2 * x.v[0];
  uy = 0.2 * x.v[1];
  ua = fabs(0.2 * x.v[2]) + fabs(0.2 * x.v[0]);

  cx = pf_matrix_zero();
  cx.m[0][0] = ux * ux;
  cx.m[1][1] = uy * uy;
  cx.m[2][2] = ua * ua;

  //printf("x = %f %f %f\n", x.v[0], x.v[1], x.v[2]);
  
  // Create a pdf with suitable characterisitics
  this->action_pdf = pf_pdf_gaussian_alloc(x, cx); 

  // Update the filter
  pf_update_action(pf, (pf_action_model_fn_t) ActionModel, this);  

  // Delete the pdf
  pf_pdf_gaussian_free(this->action_pdf);
  this->action_pdf = NULL;
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// The action model function (static method)
pf_vector_t AMCLOdom::ActionModel(AMCLOdom *self, pf_vector_t pose)
{
  pf_vector_t z, npose;
  
  z = pf_pdf_gaussian_sample(self->action_pdf);
  npose = pf_vector_coord_add(z, pose);
    
  return npose; 
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

#endif
