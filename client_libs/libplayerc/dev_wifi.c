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
 * Desc: WiFi device proxy
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
void playerc_wifi_putdata(playerc_wifi_t *device, player_msghdr_t *header,
                          player_wifi_data_t *data, size_t len);


// Create a new wifi proxy
playerc_wifi_t *playerc_wifi_create(playerc_client_t *client, int index)
{
  playerc_wifi_t *device;

  device = malloc(sizeof(playerc_wifi_t));
  memset(device, 0, sizeof(playerc_wifi_t));
  playerc_device_init(&device->info, client, PLAYER_WIFI_CODE, index,
                      (playerc_putdata_fn_t) playerc_wifi_putdata);
  
  return device;
}


// Destroy a wifi proxy
void playerc_wifi_destroy(playerc_wifi_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the wifi device
int playerc_wifi_subscribe(playerc_wifi_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the wifi device
int playerc_wifi_unsubscribe(playerc_wifi_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_wifi_putdata(playerc_wifi_t *device, player_msghdr_t *header,
                          player_wifi_data_t *data, size_t len)
{
  int i;
  
  device->link_count = (uint16_t) ntohs(data->link_count);

  for (i = 0; i < device->link_count; i++)
  {
    strncpy(device->links[i].mac, data->links[i].mac, sizeof(device->links[i].mac));
    strncpy(device->links[i].ip, data->links[i].ip, sizeof(device->links[i].ip));
    strncpy(device->links[i].essid, data->links[i].essid, sizeof(device->links[i].essid));
    device->links[i].mode = data->links[i].mode;
    device->links[i].encrypt = data->links[i].encrypt;
    device->links[i].freq = (double) (int16_t) ntohs(data->links[i].freq);
    device->links[i].qual = (int16_t) ntohs(data->links[i].qual);
    device->links[i].level = (int16_t) ntohs(data->links[i].level);
    device->links[i].noise = (int16_t) ntohs(data->links[i].noise);
  }

  return;
}

