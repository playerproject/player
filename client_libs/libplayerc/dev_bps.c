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
 * Desc: BPS device proxy
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_bps_putdata(playerc_bps_t *device, player_msghdr_t *header,
                         player_bps_data_t *data, size_t len);

// Create a new bps proxy
playerc_bps_t *playerc_bps_create(playerc_client_t *client, int index)
{
  playerc_bps_t *device;

  device = malloc(sizeof(playerc_bps_t));
  memset(device, 0, sizeof(playerc_bps_t));
  playerc_device_init(&device->info, client, PLAYER_BPS_CODE, index,
                      (playerc_putdata_fn_t) playerc_bps_putdata);
    
  return device;
}


// Destroy a bps proxy
void playerc_bps_destroy(playerc_bps_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the bps device
int playerc_bps_subscribe(playerc_bps_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the bps device
int playerc_bps_unsubscribe(playerc_bps_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_bps_putdata(playerc_bps_t *device, player_msghdr_t *header,
                         player_bps_data_t *data, size_t len)
{
}

