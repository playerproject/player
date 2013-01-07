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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */
/***************************************************************************
 * Desc: joystick proxy.
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 **************************************************************************/
#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#if !defined (WIN32)
  #include <netinet/in.h>
#endif

#include "playerc.h"
#include "error.h"

#if defined (WIN32)
  #define snprintf _snprintf
#endif

// Local declarations
void playerc_joystick_putmsg(playerc_joystick_t *device,
                           player_msghdr_t *header,
                           player_joystick_data_t *data,
                           size_t len);

// Create a new joystick proxy
playerc_joystick_t *playerc_joystick_create(playerc_client_t *client, int index)
{
  playerc_joystick_t *device;

  device = malloc(sizeof(playerc_joystick_t));
  memset(device, 0, sizeof(playerc_joystick_t));
  playerc_device_init(&device->info, client, PLAYER_JOYSTICK_CODE, index,
                      (playerc_putmsg_fn_t) playerc_joystick_putmsg);

  return device;
}


// Destroy a joystick proxy
void playerc_joystick_destroy(playerc_joystick_t *device)
{
  playerc_device_term(&device->info);
  free(device);
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
void playerc_joystick_putmsg(playerc_joystick_t *device, player_msghdr_t *header,
                            player_joystick_data_t *data, size_t len)
{
  uint32_t i;
  if((header->type == PLAYER_MSGTYPE_DATA) &&
     (header->subtype == PLAYER_JOYSTICK_DATA_STATE))
  {

    device->buttons       = data->buttons;
    device->axes_count       = data->axes_count;
    for (i = 0; i < data->axes_count ; i++)
        device->pos[i] = data->pos[i];


  }
  else
    PLAYERC_WARN2("skipping joystick message with unknown type/subtype: %s/%d\n",
                 msgtype_to_str(header->type), header->subtype);
  return;
}



