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
// Desc: AMCL gps routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include "config.h"

#include "devicetable.h"
#include "amcl_gps.h"


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLGps::AMCLGps()
{
  this->device = NULL;
  this->model = NULL;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load gps settings
int AMCLGps::Load(ConfigFile* cf, int section)
{
  // Device stuff
  this->gps_index = cf->ReadInt(section, "gps_index", -1);

  // Create the gps model
  this->model = gps_alloc();
  this->model->utm_base_e = cf->ReadTupleFloat(section, "utm_base", 0, -1);
  this->model->utm_base_n = cf->ReadTupleFloat(section, "utm_base", 1, -1);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLGps::Unload(void)
{
  gps_free(this->model);
  this->model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the gps
int AMCLGps::Setup(void)
{
  player_device_id_t id;

  // Subscribe to the Gps device
  id.code = PLAYER_GPS_CODE;
  id.index = this->gps_index;

  this->device = deviceTable->GetDriver(id);
  if (!this->device)
  {
    PLAYER_ERROR("unable to locate suitable gps device");
    return -1;
  }
  if (this->device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to gps device");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the gps
int AMCLGps::Shutdown(void)
{
  this->device->Unsubscribe(this);
  this->device = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new gps data
bool AMCLGps::GetData()
{
  size_t size;
  player_gps_data_t data;
  uint32_t tsec, tusec;

  // Get the gps device data
  size = this->device->GetData(this, (uint8_t*) &data, sizeof(data), &tsec, &tusec);
  if (size < sizeof(data))
    return false;

  if (tsec == this->tsec && tusec == this->tusec)
    return false;

  this->tsec = tsec;
  this->tusec = tusec;

  this->utm_e = ((int32_t) ntohl(data.utm_e)) / 100.0;
  this->utm_n = ((int32_t) ntohl(data.utm_n)) / 100.0;
  this->err_horz = ((int32_t) ntohl(data.err_horz)) / 1000.0;

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize from the GPS sensor model
bool AMCLGps::InitSensor(pf_t *pf, pf_vector_t mean, pf_matrix_t cov)
{
  // Check for new data
  if (!this->GetData())
    return false;

  printf("init gps %f %f %f\n",
         this->utm_e, this->utm_n, this->err_horz);

  // Pick up UTM base coordinate
  if (this->model->utm_base_e < 0 || this->model->utm_base_n < 0)
  {
    this->model->utm_base_e = this->utm_e;
    this->model->utm_base_n = this->utm_n;
    PLAYER_WARN2("UTM base coord not set; defaulting to [%.3f %.3f]",
                 this->model->utm_base_e, this->model->utm_base_n);
  }

  // Update the gps sensor model with the latest gps measurements
  gps_set_utm(this->model, this->utm_e, this->utm_n, this->err_horz);

  // Initialize the initialization routines
  gps_init_init(this->model);

  // Draw samples from the gps distribution
  pf_init(pf, (pf_init_model_fn_t) gps_init_model, this->model);

  gps_init_term(this->model);

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the gps sensor model
bool AMCLGps::UpdateSensor(pf_t *pf)
{
  // Check for new data
  if (!this->GetData())
    return false;

  printf("update gps %f %f %f\n",
         this->utm_e, this->utm_n, this->err_horz);

  // Update the gps sensor model with the latest gps measurements
  gps_set_utm(this->model, this->utm_e, this->utm_n, this->err_horz);

  // Apply the gps sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) gps_sensor_model, this->model);

  return true;
}

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLGps::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  this->fig = rtk_fig_create(canvas, NULL, 0);

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLGps::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_destroy(this->fig);
  this->fig = NULL;

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Draw the gps values
void AMCLGps::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_clear(this->fig);
  rtk_fig_color_rgb32(this->fig, 0xFF00FF);

  rtk_fig_ellipse(this->fig,
                  this->utm_e - this->model->utm_base_e,
                  this->utm_n - this->model->utm_base_n, 0,
                  this->err_horz, this->err_horz, 0);

  return;
}

#endif
