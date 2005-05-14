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
 * Desc: Joystick device proxy
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


// Create a new joystick proxy
playerc_joystick_t *playerc_joystick_create(playerc_client_t *client, int index)
{
  playerc_joystick_t *device;

  device = malloc(sizeof(playerc_joystick_t));
  memset(device, 0, sizeof(playerc_joystick_t));
  playerc_device_init(&device->info, client, PLAYER_JOYSTICK_CODE, index,
                      (playerc_putdata_fn_t) playerc_joystick_putdata,NULL,NULL);

  
  return device;
}


// Destroy a joystick proxy
void playerc_joystick_destroy(playerc_joystick_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the joystick device
int playerc_joystick_subscribe(playerc_joystick_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the joystick device
int playerc_joystick_unsubscribe(playerc_joystick_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_joystick_putdata(playerc_joystick_t *device, player_msghdr_t *header,
                              player_joystick_data_t *data, size_t len)
{
  device->px = (double) (int16_t) ntohs(data->xpos) / (double) (int16_t) ntohs(data->xscale);
  device->py = (double) (int16_t) ntohs(data->ypos) / (double) (int16_t) ntohs(data->yscale);
  device->buttons = ntohs(data->buttons);
  
  return;
}

