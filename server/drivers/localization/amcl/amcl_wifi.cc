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
// Desc: AMCL wifi routines
// Author: Andrew Howard
// Date: 6 Feb 2003
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#include "config.h"

#include "amcl.h"

#if 0

////////////////////////////////////////////////////////////////////////////////
// Load wifi settings
int AdaptiveMCL::LoadWifi(ConfigFile* cf, int section)
{
  int i;
  char key[64];
  amcl_wifi_beacon_t *beacon;

  // Wifi stuff
  this->wifi = NULL;
  this->wifi_index = cf->ReadInt(section, "wifi_index", -1);
  this->wifi_beacon_count = 0;
  for (i = 0; i < PLAYER_WIFI_MAX_LINKS; i++)
  {
    beacon = this->wifi_beacons + i;
    snprintf(key, sizeof(key), "wifi_beacon_%d", i);
    beacon->hostname = cf->ReadTupleString(section, key, 0, NULL);
    beacon->filename = cf->ReadTupleString(section, key, 1, NULL);
    if (beacon->hostname == NULL || beacon->filename == NULL)
      break;
    this->wifi_beacon_count++;
  }
  this->wifi_model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the wifi
int AdaptiveMCL::SetupWifi(void)
{
  player_device_id_t id;

  // If there is no wifi device...
  if (this->wifi_index < 0)
    return 0;

  // Subscribe to the WiFi device
  id.code = PLAYER_WIFI_CODE;
  id.index = this->wifi_index;

  this->wifi = deviceTable->GetDriver(id);
  if (!this->wifi)
  {
    PLAYER_ERROR("unable to locate suitable wifi device");
    return -1;
  }
  if (this->wifi->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to wifi device");
    return -1;
  }

  // Create the wifi model
  this->wifi_model = wifi_alloc(this->map);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shut down the wifi
int AdaptiveMCL::ShutdownWifi(void)
{
  // If there is no wifi device...
  if (this->wifi_index < 0)
    return 0;

  this->wifi->Unsubscribe(this);
  this->wifi = NULL;

  // Delete the wifi model
  wifi_free(this->wifi_model);
  this->wifi_model = NULL;

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Check for new wifi data
void AdaptiveMCL::GetWifiData(amcl_sensor_data_t *data)
{
  int i, j;
  size_t size;
  player_wifi_data_t ndata;
  player_wifi_link_t *link;
  amcl_wifi_beacon_t *beacon;

  // If there is no wifi device...
  if (this->wifi_index < 0)
  {
    data->wifi_level_count = 0;
    return;
  }

  // Get the wifi device data.
  size = this->wifi->GetData(this, (uint8_t*) &ndata, sizeof(ndata), NULL, NULL);

  data->wifi_level_count = this->wifi_beacon_count;

  for (i = 0; i < this->wifi_beacon_count; i++)
  {
    beacon = this->wifi_beacons + i;

    data->wifi_levels[i] = 0;
    for (j = 0; j < ntohs(ndata.link_count); j++)
    {
      link = ndata.links + j;

      if (strcmp(link->ip, beacon->hostname) == 0)
      {
        data->wifi_levels[i] = (int16_t) ntohs(link->level);
        //printf("[%d] [%s %s] [%d %d]\n", i, beacon->hostname,
        //       link->ip, (int) (int16_t) ntohs(link->level), data->wifi_levels[i]);
      }
    }
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Apply the wifi sensor model
bool AdaptiveMCL::UpdateWifiModel(amcl_sensor_data_t *data)
{
  // If there is no wifi device...
  if (this->wifi_index < 0)
    return false;

  // Update the wifi sensor model with the latest wifi measurements
  wifi_set_levels(this->wifi_model, data->wifi_level_count, data->wifi_levels);

  // Apply the wifi sensor model
  pf_update_sensor(this->pf, (pf_sensor_model_fn_t) wifi_sensor_model, this->wifi_model);

  return true;
}

#endif

#ifdef INCLUDE_RTKGUI

////////////////////////////////////////////////////////////////////////////////
// Draw the wifi values
void AdaptiveMCL::DrawWifiData(amcl_sensor_data_t *data)
{
  int i;
  const char *hostname;
  pf_vector_t pose;
  map_cell_t *cell;
  int olevel, mlevel;
  char ntext[128], text[1024];

  // If there is no wifi device...
  if (this->wifi_index < 0)
    return;

  // Get the robot figure pose
  rtk_fig_get_origin(this->robot_fig, pose.v + 0, pose.v + 1, pose.v + 2);

  // Get the cell at this pose
  cell = map_get_cell(this->map, pose.v[0], pose.v[1], pose.v[2]);

  text[0] = 0;
  for (i = 0; i < data->wifi_level_count; i++)
  {
    hostname = this->wifi_beacons[i].hostname;
    olevel = data->wifi_levels[i];
    mlevel = cell->wifi_levels[i];

    snprintf(ntext, sizeof(ntext), "%s %02d [%02d]\n", hostname, olevel, mlevel);
    strcat(text, ntext);
  }

  rtk_fig_clear(this->wifi_fig);
  rtk_fig_color_rgb32(this->wifi_fig, 0xFFFF00);
  rtk_fig_text(this->wifi_fig, +1, +1, 0, text);

  return;
}

#endif
