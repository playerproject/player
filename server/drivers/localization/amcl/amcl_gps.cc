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
// Desc: AMCL gps routines
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
// Load gps settings
int AdaptiveMCL::LoadGps(ConfigFile* cf, int section)
{
  // Device stuff
  this->gps = NULL;
  this->gps_index = cf->ReadInt(section, "gps_index", -1);
  
  if (this->gps_index >= 0)
  {
    // Create the gps model
    this->gps_model = gps_alloc();
    this->gps_model->utm_base_e = cf->ReadTupleFloat(section, "utm_base", 0, -1);
    this->gps_model->utm_base_n = cf->ReadTupleFloat(section, "utm_base", 1, -1);
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the gps
int AdaptiveMCL::SetupGps(void)
{
  player_device_id_t id;

  // If there is no gps device...
  if (this->gps_index < 0)
    return 0;

  // Subscribe to the Gps device
  id.code = PLAYER_GPS_CODE;
  id.index = this->gps_index;

  this->gps = deviceTable->GetDevice(id);
  if (!this->gps)
  {
    PLAYER_ERROR("unable to locate suitable gps device");
    return -1;
  }
  if (this->gps->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to gps device");
    return -1;
  }

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the gps
int AdaptiveMCL::ShutdownGps(void)
{
  // If there is no gps device...
  if (this->gps_index < 0)
    return 0;
  
  this->gps->Unsubscribe(this);
  this->gps = NULL;

  // TODO: leaks
  // Delete the gps model
  //gps_free(this->gps_model);
  //this->gps_model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new gps data
void AdaptiveMCL::GetGpsData(amcl_sensor_data_t *data)
{
  size_t size;
  player_gps_data_t ndata;
  uint32_t tsec, tusec;

  // If there is no gps device...
  if (this->gps_index < 0)
    return;

  // Get the gps device data
  size = this->gps->GetData(this, (uint8_t*) &ndata, sizeof(ndata), &tsec, &tusec);
  if (size < sizeof(ndata))
  {
    data->gps_time = 0;
    return;
  }

  data->gps_time = ntohl(tsec) + ntohl(tusec) * 1e-6;
  data->gps_utm_e = ((int32_t) ntohl(ndata.utm_e)) / 100.0;
  data->gps_utm_n = ((int32_t) ntohl(ndata.utm_n)) / 100.0;
  data->gps_err_horz = ((int32_t) ntohl(ndata.err_horz)) / 1000.0;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize from the GPS sensor model
bool AdaptiveMCL::InitGpsModel(amcl_sensor_data_t *data)
{
  if (data->gps_time == 0)
    return false;

  printf("init gps %.3f %f %f %f\n", data->gps_time,
         data->gps_utm_e, data->gps_utm_n, data->gps_err_horz);

  // Pick up UTM base coordinate
  if (this->gps_model->utm_base_e < 0 || this->gps_model->utm_base_n < 0)
  {
    this->gps_model->utm_base_e = data->gps_utm_e;
    this->gps_model->utm_base_n = data->gps_utm_n;
    PLAYER_WARN2("UTM base coord not setting; defaulting to [%.3f %.3f]",
                 this->gps_model->utm_base_e, this->gps_model->utm_base_n);
  }

  // Update the gps sensor model with the latest gps measurements
  gps_set_utm(this->gps_model, data->gps_utm_e, data->gps_utm_n, data->gps_err_horz);

  // Initialize the initialization routines
  gps_init_init(this->gps_model);
  
  // Draw samples from the gps distribution
  ::pf_init(this->pf, (pf_init_model_fn_t) gps_init_model, this->gps_model);
  
  gps_init_term(this->gps_model);
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the gps sensor model
bool AdaptiveMCL::UpdateGpsModel(amcl_sensor_data_t *data)
{
  if (data->gps_time == this->gps_time)
    return false;
  
  printf("update gps %.3f %f %f %f\n", data->gps_time,
         data->gps_utm_e, data->gps_utm_n, data->gps_err_horz);
    
  // Update the gps sensor model with the latest gps measurements
  gps_set_utm(this->gps_model, data->gps_utm_e, data->gps_utm_n, data->gps_err_horz);

  // Apply the gps sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) gps_sensor_model, this->gps_model);  

  this->gps_time = data->gps_time;
  
  return true;
}

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Draw the gps values
void AdaptiveMCL::DrawGpsData(amcl_sensor_data_t *data)
{
  rtk_fig_clear(this->gps_fig);
  rtk_fig_color_rgb32(this->gps_fig, 0xFF00FF);

  rtk_fig_ellipse(this->gps_fig,
                  data->gps_utm_e - this->gps_model->utm_base_e,
                  data->gps_utm_n - this->gps_model->utm_base_n, 0,
                  data->gps_err_horz, data->gps_err_horz, 0);
  
  return;
}

#endif
