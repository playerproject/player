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
/***************************************************************************
 * Desc: Fixed range finder (FRF) proxy
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_sonar_putdata(playerc_sonar_t *device, player_msghdr_t *header,
                           player_sonar_data_t *data, size_t len);

void playerc_sonar_putgeom(playerc_sonar_t *device, player_msghdr_t *header,
                           void *data, size_t len);

// Create a new sonar proxy
playerc_sonar_t *playerc_sonar_create(playerc_client_t *client, int index)
{
  playerc_sonar_t *device;

  device = malloc(sizeof(playerc_sonar_t));
  memset(device, 0, sizeof(playerc_sonar_t));
  playerc_device_init(&device->info, client, PLAYER_SONAR_CODE, index,
                      (playerc_putdata_fn_t) playerc_sonar_putdata,
		      (playerc_putdata_fn_t) playerc_sonar_putgeom,
		      NULL);
		      
    
  return device;
}


// Destroy a sonar proxy
void playerc_sonar_destroy(playerc_sonar_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the sonar device
int playerc_sonar_subscribe(playerc_sonar_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the sonar device
int playerc_sonar_unsubscribe(playerc_sonar_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_sonar_putdata(playerc_sonar_t *device, player_msghdr_t *header,
                           player_sonar_data_t *data, size_t len)
{
  int i;

  assert(sizeof(*data) <= len);

  device->scan_count = ntohs(data->range_count);
  for (i = 0; i < device->scan_count; i++)
    device->scan[i] = ntohs(data->ranges[i]) / 1000.0;
}

// Process incoming pushed geometry
void playerc_sonar_putgeom(playerc_sonar_t *device, player_msghdr_t *header,
                           void *data, size_t len)
{
  int i;
  player_sonar_geom_t * config = (player_sonar_geom_t *) data;

  if (len != sizeof(player_sonar_geom_t))
  {
    PLAYERC_ERR2("sonar geom has unexpected length (%d != %d)", len, sizeof(player_sonar_geom_t));
    return;
  }

  device->pose_count = htons(config->pose_count);
  for (i = 0; i < device->pose_count; i++)
  {
    device->poses[i][0] = ((int16_t) ntohs(config->poses[i][0])) / 1000.0;
    device->poses[i][1] = ((int16_t) ntohs(config->poses[i][1])) / 1000.0;
    device->poses[i][2] = ((int16_t) ntohs(config->poses[i][2])) * M_PI / 180;
  }
}

// Get the sonar geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_sonar_get_geom(playerc_sonar_t *device)
{
  int i, len;
  player_sonar_geom_t config;

  config.subtype = PLAYER_SONAR_GET_GEOM_REQ;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;

   while(device->info.freshgeom == 0)
   		playerc_client_read(device->info.client);

  return 0;
}


