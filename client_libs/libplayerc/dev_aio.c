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
/*  aio Proxy for libplayerc library.
 *  Structure based on the rest of libplayerc.
 */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_aio_putmsg(playerc_aio_t *device,
                        player_msghdr_t *header,
                        player_aio_data_t *data,
                        size_t len);


// Create a new aio proxy
playerc_aio_t *playerc_aio_create(playerc_client_t *client, int index)
{
  playerc_aio_t *device;

  device = malloc(sizeof(playerc_aio_t));
  memset(device, 0, sizeof(playerc_aio_t));
  playerc_device_init(&device->info, client, PLAYER_AIO_CODE, index,
                      (playerc_putmsg_fn_t) playerc_aio_putmsg);

  return device;
}

// Destroy a aio proxy
void playerc_aio_destroy(playerc_aio_t *device)
{
  playerc_device_term(&device->info);
  free(device->voltages);
  free(device);
}

// Subscribe to the aio device
int playerc_aio_subscribe(playerc_aio_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the aio device
int playerc_aio_unsubscribe(playerc_aio_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_aio_putmsg(playerc_aio_t *device, player_msghdr_t *header,
                        player_aio_data_t *data, size_t len)
{
  if((header->type == PLAYER_MSGTYPE_DATA) &&
     (header->subtype == PLAYER_AIO_DATA_STATE))
  {
    device->voltages_count = data->voltages_count;
    device->voltages = realloc(device->voltages, device->voltages_count * sizeof(device->voltages[0]));
    memcpy(device->voltages,
           data->voltages,
           data->voltages_count*sizeof(float));
  }
}

/* Set the output for the aio device. */
int playerc_aio_set_output(playerc_aio_t *device,
                           uint8_t id,
                           float volt)
{
  player_aio_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));

  cmd.id = id;
  cmd.voltage = volt;

  return playerc_client_write(device->info.client,
                              &device->info,
                              PLAYER_AIO_CMD_STATE,
                              &cmd,
                              NULL);
}

/** Accessor method for the aio data */
float playerc_aio_get_data(playerc_aio_t *device, uint32_t index)
{
	assert(index < device->voltages_count);
	if (index >= device->voltages_count)
		return 0;
	return device->voltages[index];
}
