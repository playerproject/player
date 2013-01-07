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
// Desc: AMCL sonar routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include "config.h"

#include <math.h>
#include "amcl.h"


#if 0
////////////////////////////////////////////////////////////////////////////////
// Load sonar settings
int AdaptiveMCL::LoadSonar(ConfigFile* cf, int section)
{
  this->sonar = NULL;
  this->sonar_index = cf->ReadInt(section, "sonar_index", -1);
  this->sonar_pose_count = 0;
  this->sonar_model = NULL;

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the sonar
int AdaptiveMCL::SetupSonar(void)
{
  int i;
  uint8_t req;
  uint16_t reptype;
  player_device_id_t id;
  player_sonar_geom_t geom;
  struct timeval tv;

  // If there is no sonar device...
  if (this->sonar_index < 0)
    return 0;

  id.code = PLAYER_SONAR_CODE;
  id.index = this->sonar_index;

  this->sonar = deviceTable->GetDriver(id);
  if (!this->sonar)
  {
    PLAYER_ERROR("unable to locate suitable sonar device");
    return -1;
  }
  if (this->sonar->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to sonar device");
    return -1;
  }

  // Get the sonar geometry
  req = PLAYER_SONAR_REQ_GET_GEOM;
  if (this->sonar->Request(&id, this, &req, 1, &reptype, &tv, &geom, sizeof(geom)) < 0)
  {
    PLAYER_ERROR("unable to get sonar geometry");
    return -1;
  }

  this->sonar_pose_count = (int16_t) ntohs(geom.pose_count);
  assert((size_t) this->sonar_pose_count <
         sizeof(this->sonar_poses) / sizeof(this->sonar_poses[0]));

  for (i = 0; i < this->sonar_pose_count; i++)
  {
    this->sonar_poses[i].v[0] = ((int16_t) ntohs(geom.poses[i][0])) / 1000.0;
    this->sonar_poses[i].v[1] = ((int16_t) ntohs(geom.poses[i][1])) / 1000.0;
    this->sonar_poses[i].v[2] = ((int16_t) ntohs(geom.poses[i][2])) * M_PI / 180.0;
  }


  // Create the sonar model
  this->sonar_model = sonar_alloc(this->map, this->sonar_pose_count, this->sonar_poses);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the sonar
int AdaptiveMCL::ShutdownSonar(void)
{
  // If there is no sonar device...
  if (this->sonar_index < 0)
    return 0;

  this->sonar->Unsubscribe(this);
  this->sonar = NULL;

  // Delete the sonar model
  sonar_free(this->sonar_model);
  this->sonar_model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new sonar data
void AdaptiveMCL::GetSonarData(amcl_sensor_data_t *data)
{
  int i;
  size_t size;
  player_sonar_data_t ndata;
  double r;

  // If there is no sonar device...
  if (this->sonar_index < 0)
  {
    data->sonar_range_count = 0;
    return;
  }

  // Get the sonar device data.
  size = this->sonar->GetData(this, (uint8_t*) &ndata, sizeof(ndata), NULL, NULL);

  data->sonar_range_count = ntohs(ndata.range_count);
  assert((size_t) data->sonar_range_count <
         sizeof(data->sonar_ranges) / sizeof(data->sonar_ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < data->sonar_range_count; i++)
  {
    r = ((int16_t) ntohs(ndata.ranges[i])) / 1000.0;
    data->sonar_ranges[i] = r;
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the sonar sensor model
bool AdaptiveMCL::UpdateSonarModel(amcl_sensor_data_t *data)
{
  int i;

  // If there is no sonar device...
  if (this->sonar_index < 0)
    return false;

  // Update the sonar sensor model with the latest sonar measurements
  sonar_clear_ranges(this->sonar_model);
  for (i = 0; i < data->sonar_range_count; i++)
    sonar_add_range(this->sonar_model, data->sonar_ranges[i]);

  // Apply the sonar sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) sonar_sensor_model, this->sonar_model);

  return true;
}
#endif

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Draw the sonar values
void AdaptiveMCL::DrawSonarData(amcl_sensor_data_t *data)
{
  int i;
  double r, b, ax, ay, bx, by;

  // If there is no sonar device...
  if (this->sonar_index < 0)
    return;

  rtk_fig_clear(this->sonar_fig);
  rtk_fig_color_rgb32(this->sonar_fig, 0xC0C080);

  for (i = 0; i < data->sonar_range_count; i++)
  {
    r = data->sonar_ranges[i];
    b = this->sonar_poses[i].v[2];

    ax = this->sonar_poses[i].v[0];
    ay = this->sonar_poses[i].v[1];

    bx = ax + r * cos(b);
    by = ay + r * sin(b);

    rtk_fig_line(this->sonar_fig, ax, ay, bx, by);
  }
  return;
}

#endif
