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
 * Desc: PTZ device proxy
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
void playerc_ptz_putdata(playerc_ptz_t *device, player_msghdr_t *header,
                         player_ptz_data_t *data, size_t len);

// Create a new ptz proxy
playerc_ptz_t *playerc_ptz_create(playerc_client_t *client, int index)
{
  playerc_ptz_t *device;

  device = malloc(sizeof(playerc_ptz_t));
  memset(device, 0, sizeof(playerc_ptz_t));
  playerc_device_init(&device->info, client, PLAYER_PTZ_CODE, index,
                      (playerc_putdata_fn_t) playerc_ptz_putdata);
    
  return device;
}


// Destroy a ptz proxy
void playerc_ptz_destroy(playerc_ptz_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the ptz device
int playerc_ptz_subscribe(playerc_ptz_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the ptz device
int playerc_ptz_unsubscribe(playerc_ptz_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_ptz_putdata(playerc_ptz_t *device, player_msghdr_t *header,
                         player_ptz_data_t *data, size_t len)
{
  device->pan = ((short) ntohs(data->pan)) * M_PI / 180;
  device->tilt = ((short) ntohs(data->tilt)) * M_PI / 180;
  device->zoom = ((short) ntohs(data->zoom)) * M_PI / 180;
}


// Set the pan, tilt and zoom values.
int playerc_ptz_set(playerc_ptz_t *device, double pan, double tilt, double zoom)
{
  player_ptz_cmd_t cmd;

  cmd.pan = htons((short) (pan * 180 / M_PI));
  cmd.tilt = htons((short) (tilt * 180 / M_PI));
  cmd.zoom = htons((short) (zoom * 180 / M_PI));

  return playerc_client_write(device->info.client, &device->info, &cmd, sizeof(cmd));
}
