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
 * Desc: Truth device proxy (works with Stage).
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_truth_putdata(playerc_truth_t *device, player_msghdr_t *header,
                           player_truth_data_t *data, size_t len);
void playerc_truth_putgeom(playerc_truth_t *device, player_msghdr_t *header,
                           player_truth_pose_t *data, size_t len);


// Create a new truth proxy
playerc_truth_t *playerc_truth_create(playerc_client_t *client, int index)
{
  playerc_truth_t *device;

  device = malloc(sizeof(playerc_truth_t));
  memset(device, 0, sizeof(playerc_truth_t));
  playerc_device_init(&device->info, client, PLAYER_TRUTH_CODE, index, 
                      (playerc_putdata_fn_t) playerc_truth_putdata,
					  (playerc_putdata_fn_t) playerc_truth_putgeom,NULL); 
  return device;
}


// Destroy a truth proxy
void playerc_truth_destroy(playerc_truth_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the truth device
int playerc_truth_subscribe(playerc_truth_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the truth device
int playerc_truth_unsubscribe(playerc_truth_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_truth_putdata(playerc_truth_t *device, player_msghdr_t *header,
                           player_truth_data_t *data, size_t len)
{
  assert(sizeof(*data) <= len);

  device->pos[0] = ((int32_t) ntohl(data->pos[0])) / 1000.0;
  device->pos[1] = ((int32_t) ntohl(data->pos[1])) / 1000.0;
  device->pos[2] = ((int32_t) ntohl(data->pos[2])) / 1000.0;

  device->rot[0] = ((int32_t) ntohl(data->rot[0])) / 1000.0;
  device->rot[1] = ((int32_t) ntohl(data->rot[1])) / 1000.0;
  device->rot[2] = ((int32_t) ntohl(data->rot[2])) / 1000.0;

  return;
}

// Process incoming geom
void playerc_truth_putgeom(playerc_truth_t *device, player_msghdr_t *header,
                           player_truth_pose_t *data, size_t len)
{
  if (len != sizeof(player_truth_pose_t))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(player_truth_pose_t));
    return;
  }

   device->pos[0] = ((int32_t) ntohl(data->pos[0])) / 1000.0;
   device->pos[1] = ((int32_t) ntohl(data->pos[1])) / 1000.0;
   device->pos[2] = ((int32_t) ntohl(data->pos[2])) / 1000.0;

   device->rot[0] = ((int32_t) ntohl(data->rot[0])) / 1000.0;
   device->rot[1] = ((int32_t) ntohl(data->rot[1])) / 1000.0;
   device->rot[2] = ((int32_t) ntohl(data->rot[2])) / 1000.0;

}


// Get the object pose.
int playerc_truth_get_pose(playerc_truth_t *device,
                           double *px, double *py, double *pz,
                           double *rx, double *ry, double *rz)
{
  int len;
  player_truth_pose_t config;

  //config.subtype = PLAYER_TRUTH_GET_POSE;

  len = playerc_client_request(device->info.client, &device->info,PLAYER_TRUTH_GET_POSE,
                               &config, 0, &config, sizeof(config));
  if (len < 0)
    return -1;
   while(device->info.freshgeom == 0)
   		playerc_client_read(device->info.client);
	*px = device->pos[0];
	*py = device->pos[1];
	*pz = device->pos[2];

	*rx = device->rot[0];
	*ry = device->rot[1];
	*rz = device->rot[2];

  return 0;
}


// Set the object pose.
int playerc_truth_set_pose(playerc_truth_t *device, 
                           double px, double py, double pz,
                           double rx, double ry, double rz)
{
  int len;
  player_truth_pose_t config;

//  config.subtype = PLAYER_TRUTH_SET_POSE;

  config.pos[0] = htonl((uint32_t) (int32_t) (px * 1000));
  config.pos[1] = htonl((uint32_t) (int32_t) (py * 1000));
  config.pos[2] = htonl((uint32_t) (int32_t) (pz * 1000));

  config.rot[0] = htonl((uint32_t) (int32_t) (rx * 1000));
  config.rot[1] = htonl((uint32_t) (int32_t) (ry * 1000));
  config.rot[2] = htonl((uint32_t) (int32_t) (rz * 1000));
  
  len = playerc_client_request(device->info.client, &device->info,PLAYER_TRUTH_SET_POSE,
                               &config, sizeof(config), &config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for a NACK

  return 0;
}


