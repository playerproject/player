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


// Create a new comms proxy
playerc_comms_t *playerc_comms_create(playerc_client_t *client, int index)
{
  playerc_comms_t *device;

  device = malloc(sizeof(playerc_comms_t));
  memset(device, 0, sizeof(playerc_comms_t));
  playerc_device_init(&device->info, client, PLAYER_COMMS_CODE, index, NULL);
    
  return device;
}


// Destroy a comms proxy
void playerc_comms_destroy(playerc_comms_t *device)
{
  playerc_device_term(&device->info);
  free(device);
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


// Send a comms message.
int playerc_comms_send(playerc_comms_t *device, void *msg, int len)
{
  player_comms_msg_t req, rep;
  int reqlen, replen;

  if (len > sizeof(req.data))
  {
    PLAYERC_ERR2("message too long; %d > %d bytes.", len, sizeof(req.data));
    return -1;
  }
  
  req.subtype = PLAYER_COMMS_SUBTYPE_SEND;
  memcpy(&req.data, msg, len);
  reqlen = sizeof(req) - sizeof(req.data) + len;

  replen =  playerc_client_request(device->info.client, &device->info,
                                   &req, reqlen, &rep, sizeof(rep));
  if (replen < 0)
    return replen;

  // TODO: process reply?
  
  return 0;
}


// Read the next comms message.
int playerc_comms_recv(playerc_comms_t *device, void *msg, int len)
{
  player_comms_msg_t req, rep;
  int reqlen, replen;

  req.subtype = PLAYER_COMMS_SUBTYPE_RECV;
  reqlen = sizeof(req.subtype);

  replen =  playerc_client_request(device->info.client, &device->info,
                                   &req, reqlen, &rep, sizeof(rep));
  if (replen < 0)
    return replen;
  if (replen == 0)
    return 0;
  if (replen > len)
  {
    PLAYERC_ERR2("message too long; %d > %d bytes.", replen, sizeof(len));
    return -1;
  }

  memcpy(msg, rep.data, replen);

  return replen;
}


