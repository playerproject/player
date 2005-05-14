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
 * Desc: Simulation device proxy
 * Author: Douglas S. Blank
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "playerc.h"
#include "playercommon.h"
#include "error.h"

// Local declarations
void playerc_simulation_putdata(playerc_simulation_t *device,
                                player_msghdr_t *header,
                                player_simulation_data_t *data, size_t len);


// Create a new simulation proxy
playerc_simulation_t *playerc_simulation_create(playerc_client_t *client, int index)
{
  playerc_simulation_t *device;

  device = malloc(sizeof(playerc_simulation_t));
  memset(device, 0, sizeof(playerc_simulation_t));
  playerc_device_init(&device->info, client, PLAYER_SIMULATION_CODE, index,
                      (playerc_putdata_fn_t) playerc_simulation_putdata,NULL,NULL);
  return device;
}


// Destroy a simulation proxy
void playerc_simulation_destroy(playerc_simulation_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the simulation device
int playerc_simulation_subscribe(playerc_simulation_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the simulation device
int playerc_simulation_unsubscribe(playerc_simulation_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_simulation_putdata(playerc_simulation_t *device, player_msghdr_t *header,
                              player_simulation_data_t *data, size_t len)
{
  // device->data 
}


// Set the target pose
int playerc_simulation_set_pose2d(playerc_simulation_t *device, char* name, double gx, double gy,
                                  double ga)
{
  player_simulation_pose2d_req_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  strncpy(cmd.name, name, PLAYER_SIMULATION_IDENTIFIER_MAXLEN);
  cmd.x = htonl((int) (gx * 1000.0));
  cmd.y = htonl((int) (gy * 1000.0));
  cmd.a = htonl((int) (ga * 180.0 / M_PI));
  //cmd.subtype = PLAYER_SIMULATION_SET_POSE2D;

  return playerc_client_request(device->info.client, &device->info, 
                                PLAYER_SIMULATION_SET_POSE2D,
                                &cmd, sizeof(cmd), &cmd, sizeof(cmd));
}

// Get the current pose
int playerc_simulation_get_pose2d(playerc_simulation_t *device, char* identifier, 
				  double *x, double *y, double *a)
{
  player_simulation_pose2d_req_t cfg;
  
  memset(&cfg, 0, sizeof(cfg));
  //cfg.subtype = PLAYER_SIMULATION_GET_POSE2D;
  strncpy( cfg.name, identifier, PLAYER_SIMULATION_IDENTIFIER_MAXLEN );
  if (playerc_client_request(device->info.client, &device->info, 
                             PLAYER_SIMULATION_GET_POSE2D,
			     &cfg, sizeof(cfg), &cfg, sizeof(cfg)) < 0)
    return (-1);
  *x =  ((int32_t)ntohl(cfg.x)) / 1e3;
  *y =  ((int32_t)ntohl(cfg.y)) / 1e3;
  *a =  DTOR((int32_t)ntohl(cfg.a));
  return 0;
}
