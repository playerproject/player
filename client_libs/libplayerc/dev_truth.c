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
 * Desc: Truth device proxy (works with Stage).
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_truth_putdata(playerc_truth_t *device, player_msghdr_t *header,
                           player_truth_data_t *data, size_t len);


// Create a new truth proxy
playerc_truth_t *playerc_truth_create(playerc_client_t *client, int robot, int index)
{
  playerc_truth_t *device;

  device = malloc(sizeof(playerc_truth_t));
  memset(device, 0, sizeof(playerc_truth_t));
  playerc_device_init(&device->info, client, robot, PLAYER_TRUTH_CODE, index, 
                      (playerc_putdata_fn_t) playerc_truth_putdata); 
  return device;
}


// Destroy a truth proxy
void playerc_truth_destroy(playerc_truth_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the truth device
int playerc_truth_subscribe(playerc_truth_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the truth device
int playerc_truth_unsubscribe(playerc_truth_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_truth_putdata(playerc_truth_t *device, player_msghdr_t *header,
                           player_truth_data_t *data, size_t len)
{
  assert(sizeof(*data) <= len);

  device->px = ntohl(data->px) / 1000.0;
  device->py = ntohl(data->py) / 1000.0;
  device->pa = ntohl(data->pa) * M_PI / 180;
}


// Get the object pose.
int playerc_truth_get_pose(playerc_truth_t *device, double *px, double *py, double *pa)
{
  int len;
  player_truth_pose_t config;

  config.subtype = PLAYER_TRUTH_GET_POSE;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  *px = ntohl(config.px) / 1000.0;
  *py = ntohl(config.py) / 1000.0;
  *pa = ntohl(config.pa) * M_PI / 180;

  return 0;
}


// Set the object pose.
int playerc_truth_set_pose(playerc_truth_t *device, double px, double py, double pa)
{
  int len;
  player_truth_pose_t config;

  config.subtype = PLAYER_TRUTH_SET_POSE;
  config.px = htonl((int) (px * 1000));
  config.py = htonl((int) (py * 1000));
  config.pa = htonl((int) (pa * 180 / M_PI)); 
  
  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config), &config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for a NACK

  return 0;
}


