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
 * Desc: GPS device proxy
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
void playerc_gps_putdata(playerc_gps_t *device, player_msghdr_t *header,
                         player_gps_data_t *data, size_t len);

// Create a new gps proxy
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index)
{
  playerc_gps_t *device;

  device = malloc(sizeof(playerc_gps_t));
  memset(device, 0, sizeof(playerc_gps_t));
  playerc_device_init(&device->info, client, PLAYER_GPS_CODE, index,
                      (playerc_putdata_fn_t) playerc_gps_putdata);
    
  return device;
}


// Destroy a gps proxy
void playerc_gps_destroy(playerc_gps_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the gps device
int playerc_gps_subscribe(playerc_gps_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the gps device
int playerc_gps_unsubscribe(playerc_gps_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_gps_putdata(playerc_gps_t *device, player_msghdr_t *header,
                         player_gps_data_t *data, size_t len)
{
  assert(sizeof(*data) <= len);

  device->utc_time = (uint32_t) ntohl(data->time_sec);
  device->utc_time += ((uint32_t) ntohl(data->time_usec)) * 1e-6;

  device->lat = (int32_t) ntohl(data->latitude) / 216000.0;
  device->lon = (int32_t) ntohl(data->longitude) / 216000.0;
  device->alt = (int32_t) ntohl(data->altitude) / 1000.0;

  device->utm_e = (int32_t) ntohl(data->utm_e) / 100.0;
  device->utm_n = (int32_t) ntohl(data->utm_n) / 100.0;

  device->hdop = (uint16_t) ntohs(data->hdop) / 10.0;
  device->err_horz = (uint32_t) ntohl(data->err_horz) / 1000.0;
  device->err_vert = (uint32_t) ntohl(data->err_vert) / 1000.0;

  device->quality = data->quality;
  device->sat_count = data->num_sats;

  return;
}

