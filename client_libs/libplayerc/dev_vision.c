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
 * Desc: Vision device proxy
 * Author: Andrew Howard
 * Date: 24 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_vision_putdata(playerc_vision_t *device, player_msghdr_t *header,
                            player_vision_data_t *data, size_t len);

// Create a new vision proxy
playerc_vision_t *playerc_vision_create(playerc_client_t *client, int index)
{
  playerc_vision_t *device;

  device = malloc(sizeof(playerc_vision_t));
  memset(device, 0, sizeof(playerc_vision_t));
  playerc_device_init(&device->info, client, PLAYER_VISION_CODE, index,
                      (playerc_putdata_fn_t) playerc_vision_putdata);
    
  return device;
}


// Destroy a vision proxy
void playerc_vision_destroy(playerc_vision_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the vision device
int playerc_vision_subscribe(playerc_vision_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the vision device
int playerc_vision_unsubscribe(playerc_vision_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_vision_putdata(playerc_vision_t *device, player_msghdr_t *header,
                           player_vision_data_t *data, size_t len)
{
}

