/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002
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
/***************************************************************************
 * Desc: Laser device proxy
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len);

// Create a new laser proxy
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index)
{
  playerc_laser_t *device;

  device = malloc(sizeof(playerc_laser_t));
  memset(device, 0, sizeof(playerc_laser_t));
  playerc_device_init(&device->info, client, PLAYER_LASER_CODE, index,
                      (playerc_putdata_fn_t) playerc_laser_putdata);

  device->pose[0] = 0.0;
  device->pose[1] = 0.0;
  device->pose[2] = 0.0;
  device->size[0] = 0.15;
  device->size[1] = 0.15;

  return device;
}


// Destroy a laser proxy
void playerc_laser_destroy(playerc_laser_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the laser device
int playerc_laser_subscribe(playerc_laser_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the laser device
int playerc_laser_unsubscribe(playerc_laser_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len)
{
  int i;
  double r, b, db;

  assert(sizeof(*data) <= len);
  
  data->min_angle = ntohs(data->min_angle);
  data->max_angle = ntohs(data->max_angle);
  data->resolution = ntohs(data->resolution);
  data->range_count = ntohs(data->range_count);

  assert(data->range_count <= ARRAYSIZE(device->scan));
  
  b = data->min_angle / 100.0 * M_PI / 180.0;
  db = data->resolution / 100.0 * M_PI / 180.0;    
  for (i = 0; i < data->range_count; i++)
  {
    r = (ntohs(data->ranges[i]) & 0x1FFF) / 1000.0;
    device->scan[i][0] = r;
    device->scan[i][1] = b;
    device->point[i][0] = r * cos(b);
    device->point[i][1] = r * sin(b);
    device->intensity[i] = ((ntohs(data->ranges[i]) & 0xE000) >> 13);
    b += db;
  }
  device->scan_count = data->range_count;
}


// Configure the laser.
int  playerc_laser_set_config(playerc_laser_t *device, double min_angle, double max_angle,
                              int resolution, int intensity)
{
  int len;
  player_laser_config_t config;

  config.subtype = PLAYER_LASER_SET_CONFIG;
  config.min_angle = htons((unsigned int) (int) (min_angle * 180.0 / M_PI * 100));
  config.max_angle = htons((unsigned int) (int) (max_angle * 180.0 / M_PI * 100));
  config.resolution = htons(resolution);
  config.intensity = (intensity ? 1 : 0);

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config), &config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for NACK
  
  return 0;
}


// Get the laser configuration.
int  playerc_laser_get_config(playerc_laser_t *device, double *min_angle, double *max_angle,
                              int *resolution, int *intensity)
{
  int len;
  player_laser_config_t config;

  config.subtype = PLAYER_LASER_GET_CONFIG;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }
  
  *min_angle = (short) ntohs(config.min_angle) / 100.0 * M_PI / 180;
  *max_angle = (short) ntohs(config.max_angle) / 100.0 * M_PI / 180;
  *resolution = ntohs(config.resolution);
  *intensity = (config.intensity ? 1 : 0);

  return 0;
}


// Get the laser geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_laser_get_geom(playerc_laser_t *device)
{
  int len;
  player_laser_geom_t config;

  config.subtype = PLAYER_LASER_GET_GEOM;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  device->pose[0] = ((int16_t) ntohs(config.pose[0])) / 1000.0;
  device->pose[1] = ((int16_t) ntohs(config.pose[1])) / 1000.0;
  device->pose[2] = ((int16_t) ntohs(config.pose[2])) * M_PI / 180;
  device->size[0] = ((int16_t) ntohs(config.size[0])) / 1000.0;
  device->size[1] = ((int16_t) ntohs(config.size[1])) / 1000.0;

  return 0;
}


