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
 * Desc: Broadcast device proxy
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


// Create a new broadcast proxy
playerc_broadcast_t *playerc_broadcast_create(playerc_client_t *client, int index)
{
  playerc_broadcast_t *device;

  device = malloc(sizeof(playerc_broadcast_t));
  memset(device, 0, sizeof(playerc_broadcast_t));
  playerc_device_init(&device->info, client, PLAYER_BROADCAST_CODE, index, NULL);
    
  return device;
}


// Destroy a broadcast proxy
void playerc_broadcast_destroy(playerc_broadcast_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the broadcast device
int playerc_broadcast_subscribe(playerc_broadcast_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the broadcast device
int playerc_broadcast_unsubscribe(playerc_broadcast_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Send a broadcast message.
int playerc_broadcast_send(playerc_broadcast_t *device, void *msg, int len)
{
  player_broadcast_msg_t req, rep;
  int reqlen, replen;

  if (len > sizeof(req.data))
  {
    PLAYERC_ERR2("message too long; %d > %d bytes.", len, sizeof(req.data));
    return -1;
  }
  
  req.subtype = PLAYER_BROADCAST_SUBTYPE_SEND;
  memcpy(&req.data, msg, len);
  reqlen = sizeof(req) - sizeof(req.data) + len;

  replen =  playerc_client_request(device->info.client, &device->info,
                                   &req, reqlen, &rep, sizeof(rep));
  if (replen < 0)
    return replen;

  // TODO: process reply?
  
  return 0;
}


// Read the next broadcast message.
int playerc_broadcast_recv(playerc_broadcast_t *device, void *msg, int len)
{
  player_broadcast_msg_t req, rep;
  int reqlen, replen;

  req.subtype = PLAYER_BROADCAST_SUBTYPE_RECV;
  reqlen = sizeof(req.subtype);

  replen =  playerc_client_request(device->info.client, &device->info,
                                   &req, reqlen, &rep, sizeof(rep));
  if (replen < 0)
    return replen;
  if (replen > len)
  {
    PLAYERC_ERR2("message too long; %d > %d bytes.", replen, sizeof(len));
    return -1;
  }

  memcpy(msg, rep.data, replen);

  return replen;
}


