/*
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002-2003
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <string.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#include "playerc.h"
#include "error.h"

#if defined (WIN32)
  #define snprintf _snprintf
#endif

// Local declarations
void playerc_ranger_putmsg(playerc_ranger_t *device, player_msghdr_t *header,
                           void *data, size_t len);

// Create a new ranger proxy
playerc_ranger_t *playerc_ranger_create(playerc_client_t *client, int index)
{
  playerc_ranger_t *device;

  device = (playerc_ranger_t*) malloc(sizeof(playerc_ranger_t));
  memset(device, 0, sizeof(playerc_ranger_t));
  playerc_device_init(&device->info, client, PLAYER_RANGER_CODE, index,
                      (playerc_putmsg_fn_t) playerc_ranger_putmsg);

  return device;
}


// Destroy a ranger proxy
void playerc_ranger_destroy(playerc_ranger_t *device)
{
  playerc_device_term(&device->info);
  if(device->ranges != NULL)
    free(device->ranges);
  if(device->intensities != NULL)
    free(device->intensities);
  if(device->element_poses != NULL)
    free(device->element_poses);
  if(device->element_sizes != NULL)
    free(device->element_sizes);
  if(device->bearings != NULL)
    free(device->bearings);
  if(device->points != NULL)
    free(device->points);
  free(device);
}


// Subscribe to the ranger device
int playerc_ranger_subscribe(playerc_ranger_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the ranger device
int playerc_ranger_unsubscribe(playerc_ranger_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Calculate bearings
void playerc_ranger_calculate_bearings(playerc_ranger_t *device)
{
  double b;
  uint32_t ii;

  device->bearings_count = device->ranges_count;
  if (device->bearings_count == 0 && device->bearings != NULL)
  {
    free(device->bearings);
    device->bearings = NULL;
  }
  else
  {
    if((device->bearings = (double *) realloc(device->bearings, device->bearings_count * sizeof(double))) == NULL)
    {
      device->bearings_count = 0;
      PLAYERC_ERR("Failed to allocate space to store bearings");
      return;
    }

    if (device->bearings_count >= device->element_count)
    {
      if (device->element_count == 1)
      {
        b = device->min_angle;
        for (ii = 0; ii < device->bearings_count; ii++)
        {
          device->bearings[ii] = b + device->device_pose.pyaw;
          b += device->angular_res;
        }
      }
      else
      {
        for (ii = 0; ii < device->element_count; ii++)
        {
          device->bearings[ii] = device->element_poses[ii].pyaw;
        }
      }
    }
  }
}


// Calculate scan points
void playerc_ranger_calculate_points(playerc_ranger_t *device)
{
  double b, r, s;
  uint32_t ii;

  device->points_count = device->ranges_count;
  if (device->points_count == 0 && device->points != NULL)
  {
    free(device->points);
    device->points = NULL;
  }
  else
  {
    if((device->points = (player_point_3d_t *) realloc(device->points, device->points_count * sizeof(player_point_3d_t))) == NULL)
    {
      device->points_count = 0;
      PLAYERC_ERR("Failed to allocate space to store points");
      return;
    }

    if (device->points_count >= device->element_count)
    {
      if (device->element_count == 1)
      {
        b = device->min_angle;
        for (ii = 0; ii < device->points_count; ii++)
        {
          r = device->ranges[ii];
          device->points[ii].px = r * cos(b);
          device->points[ii].py = r * sin(b);
          device->points[ii].pz = 0.0;
          b += device->angular_res;
        }
      }
      else
      {
        for (ii = 0; ii < device->element_count; ii++)
        {
          r = device->ranges[ii];
          s = r * cos(device->element_poses[ii].ppitch);
          device->points[ii].px = s * cos(device->element_poses[ii].pyaw) + device->element_poses[ii].px;
          device->points[ii].py = s * sin(device->element_poses[ii].pyaw) + device->element_poses[ii].py;
          device->points[ii].pz = r * sin(device->element_poses[ii].ppitch) + device->element_poses[ii].pz;
        }
      }
    }
  }
}


// Copy range data to the device
void playerc_ranger_copy_range_data(playerc_ranger_t *device, player_ranger_data_range_t *data)
{
  if (device->ranges_count != data->ranges_count || device->ranges == NULL)
  {
    // Allocate memory for the new data
    if((device->ranges = (double *) realloc(device->ranges, data->ranges_count * sizeof(double))) == NULL)
    {
      device->ranges_count = 0;
      PLAYERC_ERR("Failed to allocate space to store range data");
      return;
    }
  }
  // Copy the range data
  if (data->ranges_count > 0) memcpy(device->ranges, data->ranges, data->ranges_count * sizeof(data->ranges[0]));
  device->ranges_count = data->ranges_count;
}


// Copy intensity data to the device
void playerc_ranger_copy_intns_data(playerc_ranger_t *device, player_ranger_data_intns_t *data)
{
  if (device->intensities_count != data->intensities_count || device->intensities == NULL)
  {
    // Allocate memory for the new data
    if((device->intensities = (double *) realloc(device->intensities, data->intensities_count * sizeof(double))) == NULL)
    {
      device->intensities_count = 0;
      PLAYERC_ERR("Failed to allocate space to store intensity data");
      return;
    }
  }
  // Copy the range data
  if (data->intensities_count > 0) memcpy(device->intensities, data->intensities, data->intensities_count * sizeof(data->intensities[0]));
  device->intensities_count = data->intensities_count;
}


// Copy geometry to the device
void playerc_ranger_copy_geom(playerc_ranger_t *device, player_ranger_geom_t *geom)
{
  device->device_pose = geom->pose;
  device->device_size = geom->size;

  if(device->element_poses != NULL)
  {
    free(device->element_poses);
    device->element_poses = NULL;
  }
  if(device->element_sizes != NULL)
  {
    free(device->element_sizes);
    device->element_sizes = NULL;
  }
  device->element_count = 0;

  if(geom->element_poses_count > 0)
  {
    if((device->element_poses = (player_pose3d_t *) malloc(geom->element_poses_count * sizeof(player_pose3d_t))) == NULL)
    {
      PLAYERC_ERR("Failed to allocate space to store sensor poses");
      return;
    }
    memcpy(device->element_poses, geom->element_poses, geom->element_poses_count * sizeof(player_pose3d_t));
  }

  if (geom->element_sizes_count > 0)
  {
    if((device->element_sizes = (player_bbox3d_t *) malloc(geom->element_sizes_count * sizeof(player_bbox3d_t))) == NULL)
    {
      PLAYERC_ERR("Failed to allocate space to store sensor sizes");
      return;
    }
    memcpy(device->element_sizes, geom->element_sizes, geom->element_sizes_count * sizeof(player_bbox3d_t));
  }

  device->element_count = geom->element_poses_count;
}


// Copy config to the device
void playerc_ranger_copy_config(playerc_ranger_t *device, player_ranger_config_t *config)
{
  device->min_angle = config->min_angle;
  device->max_angle = config->max_angle;
  device->angular_res = config->angular_res;
  device->min_range = config->min_range;
  device->max_range = config->max_range;
  device->range_res = config->range_res;
  device->frequency = config->frequency;
}


// Process incoming data
void playerc_ranger_putmsg(playerc_ranger_t *device, player_msghdr_t *header,
                           void *data, size_t len)
{
  if(header->size == 0)
  {
    PLAYERC_ERR2("(putmsg) Ranger message size <= 0 in message %s/%d", msgtype_to_str(header->type), header->subtype);
    return;
  }

  if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_RANGER_DATA_RANGE))
  {
    playerc_ranger_copy_range_data(device, (player_ranger_data_range_t *) data);
    playerc_ranger_calculate_bearings(device);
    playerc_ranger_calculate_points(device);
  }
  else if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_RANGER_DATA_RANGESTAMPED))
  {
    player_ranger_data_rangestamped_t *stampedData = (player_ranger_data_rangestamped_t*) data;
    playerc_ranger_copy_range_data(device, &stampedData->data);
    if (stampedData->have_geom)
      playerc_ranger_copy_geom(device, &stampedData->geom);
    if (stampedData->have_config)
      playerc_ranger_copy_config(device, &stampedData->config);
    playerc_ranger_calculate_bearings(device);
    playerc_ranger_calculate_points(device);
  }
  else if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_RANGER_DATA_INTNS))
  {
    playerc_ranger_copy_intns_data(device, (player_ranger_data_intns_t *) data);
  }
  else if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_RANGER_DATA_INTNSSTAMPED))
  {
    player_ranger_data_intnsstamped_t *stampedData = (player_ranger_data_intnsstamped_t*) data;
    playerc_ranger_copy_intns_data(device, &stampedData->data);
    if (stampedData->have_geom)
      playerc_ranger_copy_geom(device, &stampedData->geom);
    if (stampedData->have_config)
      playerc_ranger_copy_config(device, &stampedData->config);
  }
  else if((header->type == PLAYER_MSGTYPE_DATA) && (header->subtype == PLAYER_RANGER_DATA_GEOM))
  {
    playerc_ranger_copy_geom(device, (player_ranger_geom_t *) data);
  }
  else
    PLAYERC_WARN2("Skipping ranger message with unknown type/subtype: %s/%d\n", msgtype_to_str(header->type), header->subtype);
}


// Get the ranger geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_ranger_get_geom(playerc_ranger_t *device)
{
  player_ranger_geom_t *geom;

  if(playerc_client_request(device->info.client, &device->info, PLAYER_RANGER_REQ_GET_GEOM,
                            NULL, (void**)&geom) < 0)
    return -1;

  playerc_ranger_copy_geom(device, geom);
  player_ranger_geom_t_free(geom);
  return 0;
}


// Ranger device power config
int playerc_ranger_power_config(playerc_ranger_t *device, uint8_t value)
{
  player_ranger_power_config_t req;

  req.state = value;

  if(playerc_client_request(device->info.client, &device->info, PLAYER_RANGER_REQ_POWER,
                            &req, NULL) < 0)
    return -1;

  return 0;
}

// Ranger device intensity config
int playerc_ranger_intns_config(playerc_ranger_t *device, uint8_t value)
{
  player_ranger_intns_config_t req;

  req.state = value;

  if(playerc_client_request(device->info.client, &device->info, PLAYER_RANGER_REQ_INTNS,
                            &req, NULL) < 0)
    return -1;

  return 0;
}

// Ranger set config
int playerc_ranger_set_config(playerc_ranger_t *device, double min_angle,
                              double max_angle, double angular_res,
                              double min_range, double max_range,
                              double range_res, double frequency)
{
  player_ranger_config_t config, *resp;

  config.min_angle = min_angle;
  config.max_angle = max_angle;
  config.angular_res = angular_res;
  config.min_range = min_range;
  config.max_range = max_range;
  config.range_res = range_res;
  config.frequency = frequency;

  if(playerc_client_request(device->info.client, &device->info,
                            PLAYER_RANGER_REQ_SET_CONFIG,
                            (void*)&config, (void**)&resp) < 0)
    return -1;

  playerc_ranger_copy_config(device, resp);
  player_ranger_config_t_free(resp);
  return 0;
}

// Ranger get config
int playerc_ranger_get_config(playerc_ranger_t *device, double *min_angle,
                              double *max_angle, double *angular_res,
                              double *min_range, double *max_range,
                              double *range_res, double *frequency)
{
  player_ranger_config_t *config;

  if(playerc_client_request(device->info.client, &device->info,
                            PLAYER_RANGER_REQ_GET_CONFIG,
                            NULL, (void**)&config) < 0)
    return(-1);

  playerc_ranger_copy_config(device, config);
  player_ranger_config_t_free(config);
  if (min_angle != NULL)
    *min_angle = device->min_angle;
  if (max_angle != NULL)
    *max_angle = device->max_angle;
  if (angular_res != NULL)
    *angular_res = device->angular_res;
  if (min_range != NULL)
    *min_range = device->min_range;
  if (max_range != NULL)
    *max_range = device->max_range;
  if (range_res != NULL)
    *range_res = device->range_res;
  if (frequency != NULL)
    *frequency = device->frequency;

  return 0;
}
