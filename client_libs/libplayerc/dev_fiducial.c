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
 * Desc: Fiducial device proxy
 * Author: Andrew Howard
 * Date: 15 May 2002
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

    device->fiducials[i].pos[0] = ((int32_t) ntohl(fiducial->pos[0])) / 1000.0;
    device->fiducials[i].pos[1] = ((int32_t) ntohl(fiducial->pos[1])) / 1000.0;
    device->fiducials[i].pos[2] = ((int32_t) ntohl(fiducial->pos[2])) / 1000.0;
    device->fiducials[i].rot[0] = ((int32_t) ntohl(fiducial->rot[0])) / 1000.0;
    device->fiducials[i].rot[1] = ((int32_t) ntohl(fiducial->rot[1])) / 1000.0;
    device->fiducials[i].rot[2] = ((int32_t) ntohl(fiducial->rot[2])) / 1000.0;
    device->fiducials[i].upos[0] = ((int32_t) ntohl(fiducial->upos[0])) / 1000.0;
    device->fiducials[i].upos[1] = ((int32_t) ntohl(fiducial->upos[1])) / 1000.0;
    device->fiducials[i].upos[2] = ((int32_t) ntohl(fiducial->upos[2])) / 1000.0;
    device->fiducials[i].urot[0] = ((int32_t) ntohl(fiducial->urot[0])) / 1000.0;
    device->fiducials[i].urot[1] = ((int32_t) ntohl(fiducial->urot[1])) / 1000.0;
    device->fiducials[i].urot[2] = ((int32_t) ntohl(fiducial->urot[2])) / 1000.0;

    device->fiducials[i].range = sqrt(device->fiducials[i].pos[0] * device->fiducials[i].pos[0] +
                                      device->fiducials[i].pos[1] * device->fiducials[i].pos[1]);
    device->fiducials[i].bearing = atan2(device->fiducials[i].pos[1], device->fiducials[i].pos[0]);
    device->fiducials[i].orient = device->fiducials[i].rot[2];
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
  device->size[0] = ((int16_t) ntohs(config.size[0])) / 1000.0;
  device->size[1] = ((int16_t) ntohs(config.size[1])) / 1000.0;
  device->fiducial_size[0] = ((int16_t) ntohs(config.fiducial_size[0])) / 1000.0;
  device->fiducial_size[1] = ((int16_t) ntohs(config.fiducial_size[1])) / 1000.0;

  return 0;
}
