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
 * Desc: Motor device proxy
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
void playerc_motor_putdata(playerc_motor_t *device, player_msghdr_t *header,
                              player_motor_data_t *data, size_t len);


// Create a new motor proxy
playerc_motor_t *playerc_motor_create(playerc_client_t *client, int index)
{
  playerc_motor_t *device;

  device = malloc(sizeof(playerc_motor_t));
  memset(device, 0, sizeof(playerc_motor_t));
  playerc_device_init(&device->info, client, PLAYER_MOTOR_CODE, index,
                      (playerc_putdata_fn_t) playerc_motor_putdata,NULL,NULL);

  
  return device;
}


// Destroy a motor proxy
void playerc_motor_destroy(playerc_motor_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the motor device
int playerc_motor_subscribe(playerc_motor_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the motor device
int playerc_motor_unsubscribe(playerc_motor_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_motor_putdata(playerc_motor_t *device, player_msghdr_t *header,
                              player_motor_data_t *data, size_t len)
{
  device->pt = (long) ntohl(data->theta) / 1000.0;
  device->vt = (long) ntohl(data->thetaspeed) / 1000.0;
  device->stall = data->stall;
}


// Enable/disable the motors
int playerc_motor_enable(playerc_motor_t *device, int enable)
{
  player_motor_power_config_t config;

  memset(&config, 0, sizeof(config));
//  config.request = PLAYER_MOTOR_POWER_REQ;
  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,PLAYER_MOTOR_POWER,
                                &config, sizeof(config),
                                &config, sizeof(config));    
}

// Change position/velocity control
int playerc_motor_position_control(playerc_motor_t *device, int type)
{
  player_motor_power_config_t config;

  memset(&config, 0, sizeof(config));
//  config.request = PLAYER_MOTOR_VELOCITY_MODE_REQ;
  config.value = type;

  return playerc_client_request(device->info.client, &device->info, PLAYER_MOTOR_VELOCITY_MODE,
                                &config, sizeof(config),
                                &config, sizeof(config));    
}

// Set the robot speed
int playerc_motor_set_cmd_vel(playerc_motor_t *device, double vt, int state)
{
  player_motor_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.thetaspeed = htonl((int) (vt * 1000.0));
  cmd.state = state;
  cmd.type = 0;

  return playerc_client_write(device->info.client, 
                  &device->info, &cmd, sizeof(cmd));
}


// Set the target pose
int playerc_motor_set_cmd_pose(playerc_motor_t *device, double gt, int state)
{
  player_motor_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.theta = htonl((int) (gt * 1000.0));
  cmd.state = state;
  cmd.type = 1;

  return playerc_client_write(device->info.client,
                  &device->info, &cmd, sizeof(cmd));
}

