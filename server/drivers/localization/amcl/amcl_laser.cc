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
// Desc: AMCL laser routines
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
// Load laser settings
int AdaptiveMCL::LoadLaser(ConfigFile* cf, int section)
{
  this->laser = NULL;
  this->laser_index = cf->ReadInt(section, "laser_index", -1);
  this->laser_model = NULL;
  this->laser_max_samples = cf->ReadInt(section, "laser_max_samples", 6);
  this->laser_map_err = cf->ReadLength(section, "laser_map_err", 0.05);
  this->laser_model = NULL;

  return 0;
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

  // Create the laser model
  this->laser_model = laser_alloc(this->map, this->laser_pose);
  this->laser_model->range_cov = this->laser_map_err * this->laser_map_err;

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

  // Delete the laser model
  laser_free(this->laser_model);
  this->laser_model = NULL;

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
    data->laser_range_count = 0;
    return;
  }

  // Get the laser device data.
  size = this->laser->GetData(this, (uint8_t*) &ndata, sizeof(ndata), NULL, NULL);

  b = ((int16_t) ntohs(ndata.min_angle)) / 100.0 * M_PI / 180.0;
  db = ((int16_t) ntohs(ndata.resolution)) / 100.0 * M_PI / 180.0;

  data->laser_range_count = ntohs(ndata.range_count);
  assert((size_t) data->laser_range_count <
         sizeof(data->laser_ranges) / sizeof(data->laser_ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < data->laser_range_count; i++)
  {
    r = ((int16_t) ntohs(ndata.ranges[i])) / 1000.0;
    data->laser_ranges[i][0] = r;
    data->laser_ranges[i][1] = b;
    b += db;
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the laser sensor model
void AdaptiveMCL::UpdateLaserModel(amcl_sensor_data_t *data)
{
  int i, step;
  
  // Update the laser sensor model with the latest laser measurements
  if (this->laser_max_samples >= 2)
  {
    laser_clear_ranges(this->laser_model);
    
    step = (data->laser_range_count - 1) / (this->laser_max_samples - 1);
    for (i = 0; i < data->laser_range_count; i += step)
      laser_add_range(this->laser_model, data->laser_ranges[i][0], data->laser_ranges[i][1]);

    // Apply the laser sensor model
    pf_update_sensor(this->pf, (pf_sensor_model_fn_t) laser_sensor_model, this->laser_model);
  }

  return;
}

#ifdef INCLUDE_RTKGUI

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
  for (i = 0; i < data->laser_range_count; i++)
  {
    r = data->laser_ranges[i][0];
    b = data->laser_ranges[i][1];

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
        
    step = (data->laser_range_count - 1) / (this->laser_max_samples - 1);
    for (i = 0; i < data->laser_range_count; i += step)
    {
      r = data->laser_ranges[i][0];
      b = data->laser_ranges[i][1];
      m = map_calc_range(this->map, pose.v[0], pose.v[1], pose.v[2] + b, 8.0);

      ax = 0;
      ay = 0;

      bx = ax + r * cos(b);
      by = ay + r * sin(b);
      rtk_fig_color_rgb32(this->laser_fig, 0xFF0000);
      rtk_fig_line(this->laser_fig, ax, ay, bx, by);

      /* REMOVE
      bx = ax + m * cos(b);
      by = ay + m * sin(b);
      rtk_fig_color_rgb32(this->laser_fig, 0x00FF00);
      rtk_fig_line(this->laser_fig, ax, ay, bx, by);
      */
    }

    // TESTING
    /* REMOVE
    laser_clear_ranges(this->laser_model);
    for (i = 0; i < data->laser_range_count; i += step)
      laser_add_range(this->laser_model, data->laser_ranges[i][0], data->laser_ranges[i][1]);
    //printf("prob = %f\n", laser_sensor_model(this->laser_model, pose));
    */
  }

  return;
}

#endif
