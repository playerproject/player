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
// Desc: AMCL imu  routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "amcl.h"


////////////////////////////////////////////////////////////////////////////////
// Load imu settings
int AdaptiveMCL::LoadImu(ConfigFile* cf, int section)
{
  // Device stuff
  this->imu = NULL;
  this->imu_index = cf->ReadInt(section, "imu_index", -1);

  if (this->imu_index < 0)
  {
    // No imu
    this->imu_model = NULL;
  }
  else
  {  
    // Create the imu model
    this->imu_model = imu_alloc();

    // Expect error in yaw (heading) values
    this->imu_model->err_head = cf->ReadAngle(section, "imu_err_yaw", 10 * M_PI / 180);

    // Offset added to yaw (heading) values to get UTM (true) north
    this->imu_mag_dev = cf->ReadAngle(section, "imu_mag_dev", +13 * M_PI / 180);
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the imu
int AdaptiveMCL::SetupImu(void)
{
  player_device_id_t id;

  // If there is no imu device...
  if (this->imu_index < 0)
    return 0;

  // Subscribe to the  device
  id.code = PLAYER_POSITION3D_CODE;
  id.index = this->imu_index;

  this->imu = deviceTable->GetDevice(id);
  if (!this->imu)
  {
    PLAYER_ERROR("unable to locate suitable imu device");
    return -1;
  }
  if (this->imu->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to imu device");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the imu
int AdaptiveMCL::ShutdownImu(void)
{
  // If there is no imu device...
  if (this->imu_index < 0)
    return 0;
  
  this->imu->Unsubscribe(this);
  this->imu = NULL;

  // TODO: currently leaks
  //imu_free(this->imu_model);
  //this->imu = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new imu data
void AdaptiveMCL::GetImuData(amcl_sensor_data_t *data)
{
  size_t size;
  player_position3d_data_t ndata;

  // If there is no imu device...
  if (this->imu_index < 0)
    return;

  // Get the imu device data
  size = this->imu->GetData(this, (uint8_t*) &ndata, sizeof(ndata), NULL, NULL);

  data->imu_utm_head = ((int32_t) ntohl(ndata.yaw)) / 3600.0 * M_PI / 180;
  data->imu_utm_head += this->imu_mag_dev;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the imu sensor model
bool AdaptiveMCL::UpdateImuModel(amcl_sensor_data_t *data)
{
  // If there is no imu device...
  if (this->imu_index < 0)
    return true;

  printf("update imu %f\n", data->imu_utm_head * 180 / M_PI);
  
  // Update the imu sensor model with the latest imu measurements
  imu_set_utm(this->imu_model, data->imu_utm_head);

  // Apply the imu sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) imu_sensor_model, this->imu_model);  
  
  return true;
}

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Draw the imu values
void AdaptiveMCL::DrawImuData(amcl_sensor_data_t *data)
{
  double ox, oy, oa;
  
  // If there is no imu device...
  if (this->imu_index < 0)
    return;

  rtk_fig_clear(this->imu_fig);
  rtk_fig_color_rgb32(this->imu_fig, 0xFF00FF);
  rtk_fig_get_origin(this->robot_fig, &ox, &oy, &oa);
  rtk_fig_arrow(this->imu_fig, ox, oy, data->imu_utm_head + M_PI / 2, 1.0, 0.20);

  return;
}

#endif
