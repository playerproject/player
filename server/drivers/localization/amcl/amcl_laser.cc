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

#define PLAYER_ENABLE_MSG 1

#include <math.h>
#include "devicetable.h"
#include "amcl_laser.h"


////////////////////////////////////////////////////////////////////////////////
// Default constructor
AMCLLaser::AMCLLaser()
{
  this->device = NULL;
  this->model = NULL;
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Load laser settings
int AMCLLaser::Load(ConfigFile* cf, int section)
{
  const char *map_filename;
  double map_scale;
  int map_negate;
  
  // Device stuff
  this->laser_index = cf->ReadInt(section, "laser_index", -1);

  // Get the map settings
  map_filename = cf->ReadFilename(section, "laser_map", NULL);
  map_scale = cf->ReadLength(section, "laser_map_scale", 0.05);
  map_negate = cf->ReadInt(section, "laser_map_negate", 0);

  // Create the map
  this->map = map_alloc();
  PLAYER_MSG1("loading map file [%s]", map_filename);
  if (map_load_occ(this->map, map_filename, map_scale, map_negate) != 0)
    return -1;
  
  // Create the laser model
  this->model = laser_alloc(map);
  this->model->range_cov = pow(cf->ReadLength(section, "laser_map_err", 0.05), 2);
  this->max_ranges = cf->ReadInt(section, "laser_max_ranges", 6);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Unload the model
int AMCLLaser::Unload(void)
{
  laser_free(this->model);
  this->model = NULL;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the laser
int AMCLLaser::Setup(void)
{
  uint8_t req;
  uint16_t reptype;
  player_device_id_t id;
  player_laser_geom_t geom;
  struct timeval tv;

  // Subscribe to the Laser device
  id.code = PLAYER_LASER_CODE;
  id.index = this->laser_index;

  this->device = deviceTable->GetDevice(id);
  if (!this->device)
  {
    PLAYER_ERROR("unable to locate suitable laser device");
    return -1;
  }
  if (this->device->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to laser device");
    return -1;
  }
  
  // Get the laser geometry
  req = PLAYER_LASER_GET_GEOM;
  if (this->device->Request(&id, this, &req, 1, &reptype, &tv, &geom, sizeof(geom)) < 0)
  {
    PLAYER_ERROR("unable to get laser geometry");
    return -1;
  }

  // Set the laser pose relative to the robot
  this->model->laser_pose.v[0] = ((int16_t) ntohl(geom.pose[0])) / 1000.0;
  this->model->laser_pose.v[1] = ((int16_t) ntohl(geom.pose[1])) / 1000.0;
  this->model->laser_pose.v[2] = ((int16_t) ntohl(geom.pose[2])) * M_PI / 180.0;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the laser
int AMCLLaser::Shutdown(void)
{  
  this->device->Unsubscribe(this);
  this->device = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new laser data
bool AMCLLaser::GetData()
{
  int i;
  size_t size;
  player_laser_data_t data;
  uint32_t tsec, tusec;
  double r, b, db;

  // Get the laser device data.
  size = this->device->GetData(this, (uint8_t*) &data, sizeof(data), &tsec, &tusec);
  if (size == 0)
    return false;

  if (tsec == this->tsec && tusec == this->tusec)
    return false;

  b = ((int16_t) ntohs(data.min_angle)) / 100.0 * M_PI / 180.0;
  db = ((int16_t) ntohs(data.resolution)) / 100.0 * M_PI / 180.0;

  this->range_count = ntohs(data.range_count);
  assert((size_t) this->range_count < sizeof(this->ranges) / sizeof(this->ranges[0]));

  // Read and byteswap the range data
  for (i = 0; i < this->range_count; i++)
  {
    r = ((int16_t) ntohs(data.ranges[i])) / 1000.0;
    this->ranges[i][0] = r;
    this->ranges[i][1] = b;
    b += db;
  }
  
  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the laser sensor model
bool AMCLLaser::UpdateSensor(pf_t *pf)
{
  int i, step;
  
  // Check for new data
  if (!this->GetData())
    return false;
  
  printf("update laser\n");

  if (this->max_ranges < 2)
    return false;
  
  // Update the laser sensor model with the latest laser measurements
  laser_clear_ranges(this->model);
    
  step = (this->range_count - 1) / (this->max_ranges - 1);
  for (i = 0; i < this->range_count; i += step)
    laser_add_range(this->model, this->ranges[i][0], this->ranges[i][1]);

  // Apply the laser sensor model
  pf_update_sensor(pf, (pf_sensor_model_fn_t) laser_sensor_model, this->model);
  
  return true;
}

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Setup the GUI
void AMCLLaser::SetupGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{  
  this->fig = rtk_fig_create(canvas, robot_fig, 0);

  // Draw the laser map
  this->map_fig = rtk_fig_create(canvas, NULL, -50);
  map_draw_occ(this->map, this->map_fig);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the GUI
void AMCLLaser::ShutdownGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  rtk_fig_destroy(this->map_fig);
  rtk_fig_destroy(this->fig);
  this->map_fig = NULL;
  this->fig = NULL;

  return;
}

////////////////////////////////////////////////////////////////////////////////
// Draw the laser values
void AMCLLaser::UpdateGUI(rtk_canvas_t *canvas, rtk_fig_t *robot_fig)
{
  int i;
  double r, b, ax, ay, bx, by;

  rtk_fig_clear(this->fig);

  // Draw the complete scan
  rtk_fig_color_rgb32(this->fig, 0x8080FF);
  for (i = 0; i < this->range_count; i++)
  {
    r = this->ranges[i][0];
    b = this->ranges[i][1];

    ax = 0;
    ay = 0;
    bx = ax + r * cos(b);
    by = ay + r * sin(b);

    rtk_fig_line(this->fig, ax, ay, bx, by);
  }

  // TODO: FIX
  /*
  // Get the robot figure pose
  rtk_fig_get_origin(robot_fig, pose.v + 0, pose.v + 1, pose.v + 2);

  if (this->max_ranges < 2)
    continue;

  // Draw the significant part of the scan
  step = (this->range_count - 1) / (this->laser_max_ranges - 1);
  for (i = 0; i < this->range_count; i += step)
  {
    r = this->ranges[i][0];
    b = this->ranges[i][1];
    m = map_calc_range(this->map, pose.v[0], pose.v[1], pose.v[2] + b, 8.0);

    ax = 0;
    ay = 0;

    bx = ax + r * cos(b);
    by = ay + r * sin(b);
    rtk_fig_color_rgb32(this->fig, 0xFF0000);
    rtk_fig_line(this->fig, ax, ay, bx, by);
  }
  */
  
  return;
}

#endif



