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
 * Desc: Position3d device proxy
 * Author: Andrew Howard
 * Date: 13 May 2002
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


// Create a new position3d proxy
playerc_position3d_t *playerc_position3d_create(playerc_client_t *client, int index)
{
  playerc_position3d_t *device;

  device = malloc(sizeof(playerc_position3d_t));
  memset(device, 0, sizeof(playerc_position3d_t));
  playerc_device_init(&device->info, client, PLAYER_POSITION3D_CODE, index,
                      (playerc_putdata_fn_t) playerc_position3d_putdata);

  
  return device;
}


// Destroy a position3d proxy
void playerc_position3d_destroy(playerc_position3d_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the position3d device
int playerc_position3d_subscribe(playerc_position3d_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the position3d device
int playerc_position3d_unsubscribe(playerc_position3d_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_position3d_putdata(playerc_position3d_t *device, player_msghdr_t *header,
                                player_position3d_data_t *data, size_t len)
{
  device->pos_x = (long) ntohl(data->xpos) / 1000.0;
  device->pos_y = (long) ntohl(data->ypos) / 1000.0;
  device->pos_z = (long) ntohl(data->zpos) / 1000.0;

  device->pos_roll = (long) ntohl(data->roll)  / 1000.0;
  device->pos_pitch = (long) ntohl(data->pitch) / 1000.0;
  device->pos_yaw = (long) ntohl(data->yaw)  / 1000.0;

  device->vel_x = (long) ntohl(data->xspeed) / 1000.0;
  device->vel_y = (long) ntohl(data->yspeed) / 1000.0;
  device->vel_z = (long) ntohl(data->zspeed) / 1000.0;

  device->vel_roll = (long) ntohl(data->rollspeed) / 1000.0;
  device->vel_pitch = (long) ntohl(data->pitchspeed) / 1000.0;
  device->vel_yaw = (long) ntohl(data->yawspeed) / 1000.0;
  
  device->stall = data->stall;
}


// Enable/disable the motors
int playerc_position3d_enable(playerc_position3d_t *device, int enable)
{
  player_position3d_power_config_t config;

  memset(&config, 0, sizeof(config));
  config.request = PLAYER_POSITION3D_MOTOR_POWER_REQ;
  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,
                                &config, sizeof(config),
                                &config, sizeof(config));    
}


// Get the position3d geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_position3d_get_geom(playerc_position3d_t *device)
{
  int len;
  player_position_geom_t config;

  memset(&config, 0, sizeof(config));
  config.subtype = PLAYER_POSITION3D_GET_GEOM_REQ;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  device->pose[0] = ((int16_t) ntohs(config.pose[0])) / 1000.0;
  device->pose[1] = ((int16_t) ntohs(config.pose[1])) / 1000.0;
  device->pose[2] = ((int16_t) ntohs(config.pose[2])) / 1000.0;

  device->pose[3] = ((int16_t) ntohs(config.pose[3]))  / 1000.0;
  device->pose[4] = ((int16_t) ntohs(config.pose[4])) / 1000.0;
  device->pose[5] = ((int16_t) ntohs(config.pose[5])) / 1000.0;

  device->size[0] = ((int16_t) ntohs(config.size[0])) / 1000.0;
  device->size[1] = ((int16_t) ntohs(config.size[1])) / 1000.0;
  device->size[2] = ((int16_t) ntohs(config.size[2])) / 1000.0;

  return 0;
}


// Set the robot speed
int playerc_position3d_set_velocity(playerc_position3d_t *device, 
                     double vx, double vy, double vz,
                     double vr, double vp, double vt, int state)
{
  player_position3d_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.xspeed = htonl((int) (vx * 1000.0));
  cmd.yspeed = htonl((int) (vy * 1000.0));
  cmd.zspeed = htonl((int) (vz * 1000.0));

  cmd.rollspeed = htonl((int) (vr * 1000.0));
  cmd.pitchspeed = htonl((int) (vp * 1000.0));
  cmd.yawspeed = htonl((int) (vt * 1000.0));

  cmd.state = state;

  return playerc_client_write(device->info.client,
                     &device->info, &cmd, sizeof(cmd));
}


// Set the target pose
int playerc_position3d_set_pose(playerc_position3d_t *device,
                        double gx, double gy, double gz,
                        double gr, double gp, double gt)
{
  player_position3d_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.xpos = htonl((int) (gx * 1000.0));
  cmd.ypos = htonl((int) (gy * 1000.0));
  cmd.zpos = htonl((int) (gz * 1000.0));

  cmd.roll = htonl((int) (gr * 1000.0));
  cmd.pitch = htonl((int) (gp * 1000.0));
  cmd.yaw = htonl((int) (gt * 1000.0));

  return playerc_client_write(device->info.client,
                     &device->info, &cmd, sizeof(cmd));
}

/** For compatibility with old position3d interface */
int playerc_position3d_set_speed(playerc_position3d_t *device,
                                 double vx, double vy, double vz, int state)
{
  return playerc_position3d_set_velocity(device,vx,vy,vz,0,0,0, state);
}

/** For compatibility with old position3d interface */
int playerc_position3d_set_cmd_pose(playerc_position3d_t *device,
                                    double gx, double gy, double gz)
{
  return playerc_position3d_set_pose(device,gx,gy,gz,0,0,0);
}

