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
 * Desc: Broadcast device proxy
 * Author: Andrew Howard
 * Date: 13 May 2002
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
void playerc_comms_putdata(playerc_comms_t *device, player_msghdr_t *header,
                           void *data, size_t len);


// Create a new comms proxy
playerc_comms_t *playerc_comms_create(playerc_client_t *client, int index)
{
  playerc_comms_t *device;

  device = malloc(sizeof(playerc_comms_t));
  memset(device, 0, sizeof(playerc_comms_t));
  playerc_device_init(&device->info, client, PLAYER_COMMS_CODE, index,
                      (playerc_putdata_fn_t) playerc_comms_putdata);

  device->msg_len = 0;
    
  return device;
}


// Destroy a comms proxy
void playerc_comms_destroy(playerc_comms_t *device)
{
  playerc_device_term(&device->info);
  free(device);
  return;
}


// Subscribe to the comms device
int playerc_comms_subscribe(playerc_comms_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the comms device
int playerc_comms_unsubscribe(playerc_comms_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_comms_putdata(playerc_comms_t *device, player_msghdr_t *header,
                           void *data, size_t len)
{
  if (len > PLAYER_MAX_MESSAGE_SIZE)
  {
    PLAYERC_ERR2("incoming message too long; %d > %d bytes.", len, PLAYER_MAX_MESSAGE_SIZE);
    return;
  }
  device->msg_len = len;
  memcpy(device->msg, data, len);
  return;
}


// Send a message
int playerc_comms_send(playerc_comms_t *device, void *msg, int len)
{
  if (len > PLAYER_MAX_MESSAGE_SIZE)
  {
    PLAYERC_ERR2("outgoing message too long; %d > %d bytes.", len, PLAYER_MAX_MESSAGE_SIZE);
    return -1;
  }
  return playerc_client_write(device->info.client, &device->info, msg, len);
}
