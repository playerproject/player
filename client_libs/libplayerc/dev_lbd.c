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
 * Desc: LBD (laser beacon detector) device proxy
 * Author: Andrew Howard
 * Date: 15 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_lbd_putdata(playerc_lbd_t *device, player_msghdr_t *header,
                         player_laserbeacon_data_t *data, size_t len);

// Create a new lbd proxy
playerc_lbd_t *playerc_lbd_create(playerc_client_t *client, int index)
{
  playerc_lbd_t *device;

  device = malloc(sizeof(playerc_lbd_t));
  memset(device, 0, sizeof(playerc_lbd_t));
  playerc_device_init(&device->info, client, PLAYER_LASERBEACON_CODE, index,
                      (playerc_putdata_fn_t) playerc_lbd_putdata);

  device->pose[0] = 0.0;
  device->pose[1] = 0.0;
  device->pose[2] = 0.0;
  
  return device;
}


// Destroy a lbd proxy
void playerc_lbd_destroy(playerc_lbd_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the lbd device
int playerc_lbd_subscribe(playerc_lbd_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the lbd device
int playerc_lbd_unsubscribe(playerc_lbd_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_lbd_putdata(playerc_lbd_t *device, player_msghdr_t *header,
                         player_laserbeacon_data_t *data, size_t len)
{
  int i;

  device->beacon_count = ntohs(data->count);

  for (i = 0; i < device->beacon_count; i++)
  {
    device->beacons[i].id = data->beacon[i].id;
    device->beacons[i].range = ntohs(data->beacon[i].range) / 1000.0;
    device->beacons[i].bearing = ((int) (int16_t) ntohs(data->beacon[i].bearing)) * M_PI / 180;
    device->beacons[i].orient = ((int) (int16_t) ntohs(data->beacon[i].orient)) * M_PI / 180;
  }
}


// Get the lbd geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_lbd_get_geom(playerc_lbd_t *device)
{
  int len;
  player_laserbeacon_geom_t config;

  config.subtype = PLAYER_LASERBEACON_GET_GEOM;

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

  return 0;
}


// Set the device configuration
int playerc_lbd_set_config(playerc_lbd_t *device,
                           int bit_count, double bit_width)
{
  int len;
  player_laserbeacon_config_t config;

  // Get the current device configuration.
  config.subtype = PLAYER_LASERBEACON_GET_CONFIG;
  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype),
                               &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  // Change the bit size and the number of bits
  config.subtype = PLAYER_LASERBEACON_SET_CONFIG;
  config.bit_count = bit_count;
  config.bit_size = htons((uint16_t)(bit_width * 1000));
  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config), &config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for a NACK

  return 0;
}


// Get the device configuration
int playerc_lbd_get_config(playerc_lbd_t *device,
                           int *bit_count, double *bit_width)
{
  int len;
  player_laserbeacon_config_t config;

  // Get the current device configuration.
  config.subtype = PLAYER_LASERBEACON_GET_CONFIG;
  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  *bit_count = config.bit_count;
  *bit_width = ntohs(config.bit_size) / 1000.0;
  
  return 0;
}
