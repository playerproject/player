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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
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

#include "config.h"

#include "devicetable.h"
#include "amcl_imu.h"


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLImu::AMCLImu()
{
  this->device = NULL;
  this->model = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load imu settings
int AMCLImu::Load(ConfigFile* cf, int section)
{
  // Device stuff
  this->imu_index = cf->ReadInt(section, "imu_index", -1);

  // Create the imu model
  this->model = imu_alloc();

  // Offset added to yaw (heading) values to get UTM (true) north
  this->utm_mag_dev = cf->ReadAngle(section, "imu_mag_dev", +13 * M_PI / 180);

  // Expect error in yaw (heading) values
  this->model->err_head = cf->ReadAngle(section, "imu_err_yaw", 10 * M_PI / 180);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLImu::Unload(void)
{
  imu_free(this->model);
  this->model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the imu
int AMCLImu::Setup(void)
{
  player_device_id_t id;

  // Subscribe to the Imu device
  id.code = PLAYER_POSITION3D_CODE;
  id.index = this->imu_index;

  this->device = deviceTable->GetDriver(id);
  if (!this->device)
  {
    PLAYER_ERROR("unable to locate suitable imu device");
    return -1;
  }
  if (this->device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to imu device");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the imu
int AMCLImu::Shutdown(void)
{
  this->device->Unsubscribe(this);
  this->device = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new imu data
bool AMCLImu::GetData()
{
  size_t size;
  player_position3d_data_t data;
  uint32_t tsec, tusec;

  // Get the imu device data
  size = this->device->GetData(this, (uint8_t*) &data, sizeof(data), &tsec, &tusec);
  if (size < sizeof(data))
    return false;

  if (tsec == this->tsec && tusec == this->tusec)
    return false;

  this->tsec = tsec;
  this->tusec = tusec;

  this->utm_head = ((int32_t) ntohl(data.yaw)) / 3600.0 * M_PI / 180;
  this->utm_head += this->utm_mag_dev;

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the imu sensor model
bool AMCLImu::UpdateSensor(pf_t *pf)
{
  // Check for new data
  if (!this->GetData())
    return false;

  printf("update imu %f\n", this->utm_head * 180 / M_PI);

  // Update the imu sensor model with the latest imu measurements
  imu_set_utm(this->model, this->utm_head);

  // Apply the imu sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) imu_sensor_model, this->model);

  return true;
}

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLImu::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  this->fig = rtk_fig_create(canvas, NULL, 0);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLImu::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_destroy(this->fig);
  this->fig = NULL;

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Draw the imu values
void AMCLImu::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  double ox, oy, oa;

  rtk_fig_clear(this->fig);
  rtk_fig_color_rgb32(this->fig, 0xFF00FF);
  rtk_fig_get_origin(robot_fig, &ox, &oy, &oa);
  rtk_fig_arrow(this->fig, ox, oy, this->utm_head + M_PI / 2, 1.0, 0.20);

  return;
}

#endif

