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
void playerc_fiducial_putdata(playerc_fiducial_t *device, player_msghdr_t *header,
                              player_fiducial_data_t *data, size_t len);

// Create a new fiducial proxy
playerc_fiducial_t *playerc_fiducial_create(playerc_client_t *client, int index)
{
  playerc_fiducial_t *device;

  device = malloc(sizeof(playerc_fiducial_t));
  memset(device, 0, sizeof(playerc_fiducial_t));
  playerc_device_init(&device->info, client, PLAYER_FIDUCIAL_CODE, index,
                      (playerc_putdata_fn_t) playerc_fiducial_putdata);

  device->pose[0] = 0.0;
  device->pose[1] = 0.0;
  device->pose[2] = 0.0;
  
  return device;
}


// Destroy a fiducial proxy
void playerc_fiducial_destroy(playerc_fiducial_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the fiducial device
int playerc_fiducial_subscribe(playerc_fiducial_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the fiducial device
int playerc_fiducial_unsubscribe(playerc_fiducial_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_fiducial_putdata(playerc_fiducial_t *device, player_msghdr_t *header,
                         player_fiducial_data_t *data, size_t len)
{
  int i;
  player_fiducial_item_t *fiducial;

  device->fiducial_count = ntohs(data->count);

  for (i = 0; i < device->fiducial_count; i++)
  {
    fiducial = data->fiducials + i;
    device->fiducials[i].id = (int16_t) ntohs(fiducial->id);
    device->fiducials[i].range = (int16_t) ntohs(fiducial->pose[0]) / 1000.0;
    device->fiducials[i].bearing = (int16_t) ntohs(fiducial->pose[1]) * M_PI / 180;
    device->fiducials[i].orient = (int16_t) ntohs(fiducial->pose[2]) * M_PI / 180;
  }
}


// Get the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_fiducial_get_geom(playerc_fiducial_t *device)
{
  int len;
  player_fiducial_geom_t config;

  config.subtype = PLAYER_FIDUCIAL_GET_GEOM;

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
int playerc_fiducial_set_config(playerc_fiducial_t *device,
                           int bit_count, double bit_width)
{
  int len;
  player_fiducial_laserbarcode_config_t config;

  // Get the current device configuration.
  config.subtype = PLAYER_FIDUCIAL_LASERBARCODE_GET_CONFIG;
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
  config.subtype = PLAYER_FIDUCIAL_LASERBARCODE_SET_CONFIG;
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
int playerc_fiducial_get_config(playerc_fiducial_t *device,
                           int *bit_count, double *bit_width)
{
  int len;
  player_fiducial_laserbarcode_config_t config;

  // Get the current device configuration.
  config.subtype = PLAYER_FIDUCIAL_LASERBARCODE_GET_CONFIG;
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
