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
  assert(sizeof(*data) <= len);
  
  device->px = (int32_t) ntohl(data->px) / 1000.0;
  device->py = (int32_t) ntohl(data->py) / 1000.0;
  device->pa = (int32_t) ntohl(data->pa) * M_PI / 180;
  device->ux = (int32_t) ntohl(data->ux) / 1000.0;
  device->uy = (int32_t) ntohl(data->uy) / 1000.0;
  device->ua = (int32_t) ntohl(data->ua) * M_PI / 180;
  device->err = (int32_t) ntohl(data->err) * 1e6;
}


// Set the pose a beacon in global coordinates.
int  playerc_bps_set_beacon(playerc_bps_t *device, int id, double px, double py, double pa,
                            double ux, double uy, double ua)
{
  int len;
  player_bps_beacon_t beacon;

  beacon.subtype = PLAYER_BPS_SET_BEACON;
  beacon.id = id;
  beacon.px = htonl((int32_t) (px * 1000));
  beacon.py = htonl((int32_t) (py * 1000));
  beacon.pa = htonl((int32_t) (pa * 180 / M_PI));
  beacon.ux = htonl((int32_t) (ux * 1000));
  beacon.uy = htonl((int32_t) (uy * 1000));
  beacon.ua = htonl((int32_t) (ua * 180 / M_PI));

  len = playerc_client_request(device->info.client, &device->info,
                               &beacon, sizeof(beacon), &beacon, sizeof(beacon));
  if (len < 0)
    return -1;

  // TODO: check for NACK
  
  return 0;
}

// Get the pose of a beacon in global coordinates.
int  playerc_bps_get_beacon(playerc_bps_t *device, int id, double *px, double *py, double *pa,
                            double *ux, double *uy, double *ua)
{
  int len;
  player_bps_beacon_t beacon;

  beacon.subtype = PLAYER_BPS_GET_BEACON;
  beacon.id = id;

  len = playerc_client_request(device->info.client, &device->info,
                               &beacon, sizeof(beacon), &beacon, sizeof(beacon));
  if (len < 0)
    return -1;
  if (len != sizeof(beacon))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(beacon));
    return -1;
  }

  *px = (int32_t) ntohl(beacon.px) / 1000.0;
  *py = (int32_t) ntohl(beacon.py) / 1000.0;
  *pa = (int32_t) ntohl(beacon.pa) * M_PI / 180;
  *ux = (int32_t) ntohl(beacon.ux) / 1000.0;
  *uy = (int32_t) ntohl(beacon.uy) / 1000.0;
  *ua = (int32_t) ntohl(beacon.ua) * M_PI / 180;

  return 0;
}


