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
void playerc_motor_putmsg(playerc_motor_t *device,
                          player_msghdr_t *header,
                          player_motor_data_t *data, size_t len);

// Create a new motor proxy
playerc_motor_t *playerc_motor_create(playerc_client_t *client, int index)
{
  playerc_motor_t *device;

  device = malloc(sizeof(playerc_motor_t));
  memset(device, 0, sizeof(playerc_motor_t));
  playerc_device_init(&device->info, client, PLAYER_MOTOR_CODE, index,
                      (playerc_putmsg_fn_t) playerc_motor_putmsg);


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
void playerc_motor_putmsg(playerc_motor_t *device,
                          player_msghdr_t *header,
                          player_motor_data_t *data, size_t len)
{
  if((header->type == PLAYER_MSGTYPE_DATA) &&
     (header->subtype == PLAYER_MOTOR_DATA_STATE))
  {
    device->pt = data->pos;
    device->vt = data->vel;

    device->limits = data->limits;
    device->stall = data->stall;
  }
  else
    PLAYERC_WARN2("skipping motor message with unknown type/subtype: %s/%d\n",
                 msgtype_to_str(header->type), header->subtype);
}

// Enable/disable the motors
int
playerc_motor_enable(playerc_motor_t *device, int enable)
{
  player_motor_power_config_t config;

  config.state = enable;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_MOTOR_REQ_POWER,
                                &config, NULL, 0));
}

int
playerc_motor_position_control(playerc_motor_t *device, int type)
{
  player_motor_position_mode_req_t config;

  config.value = type;

  return(playerc_client_request(device->info.client, &device->info,
                                PLAYER_MOTOR_REQ_VELOCITY_MODE,
                                &config, NULL, 0));
}

// Set the robot speed
int
playerc_motor_set_cmd_vel(playerc_motor_t *device,
                          double vt, int state)
{
  player_motor_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.vel = vt;
  cmd.state = state;
  cmd.type = 0;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_MOTOR_CMD_STATE,
                              &cmd, NULL);
}

// Set the target pose
int
playerc_motor_set_cmd_pose(playerc_motor_t *device,
                           double gt, int state)
{
  player_motor_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.pos = gt;
  cmd.state = state;
  cmd.type = 1;

  return playerc_client_write(device->info.client, &device->info,
                              PLAYER_MOTOR_CMD_STATE,
                              &cmd, NULL);
}

// Set the odometry offset
int
playerc_motor_set_odom(playerc_motor_t *device, double ot)
{
  player_motor_set_odom_req_t req;

  req.theta = ot;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_MOTOR_REQ_SET_ODOM,
                                &req, NULL, 0));
}

void playerc_motor_print( playerc_motor_t * device,
             const char* prefix )
{
  if( prefix )
    printf( "%s: ", prefix );

  printf("#time\t\tpt\tvt\tlimits\tstall\n"
         "%14.3f\t%.3f\t%.3f\t%d\t%.d\n",
    device->info.datatime,
    device->pt,
    device->vt,
    device->limits,
    device->stall );
}
