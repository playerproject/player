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
 * Desc: Position device proxy
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
void playerc_position_putdata(playerc_position_t *device, player_msghdr_t *header,
                              player_position_data_t *data, size_t len);


// Create a new position proxy
playerc_position_t *playerc_position_create(playerc_client_t *client, int index)
{
  playerc_position_t *device;

  device = malloc(sizeof(playerc_position_t));
  memset(device, 0, sizeof(playerc_position_t));
  playerc_device_init(&device->info, client, PLAYER_POSITION_CODE, index,
                      (playerc_putdata_fn_t) playerc_position_putdata);

  
  return device;
}


// Destroy a position proxy
void playerc_position_destroy(playerc_position_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the position device
int playerc_position_subscribe(playerc_position_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the position device
int playerc_position_unsubscribe(playerc_position_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_position_putdata(playerc_position_t *device, player_msghdr_t *header,
                              player_position_data_t *data, size_t len)
{
  device->px = (long) ntohl(data->xpos) / 1000.0;
  device->py = (long) ntohl(data->ypos) / 1000.0;
  device->pa = (long) ntohl(data->yaw) * M_PI / 180.0;
  device->pa = atan2(sin(device->pa), cos(device->pa));
  
  device->vx = (long) ntohl(data->xspeed) / 1000.0;
  device->vy = (long) ntohl(data->yspeed) / 1000.0;
  device->va = (long) ntohl(data->yawspeed) * M_PI / 180.0;
  
  device->stall = data->stall;
}


// Enable/disable the motors
int playerc_position_enable(playerc_position_t *device, int enable)
{
  player_position_power_config_t config;

  memset(&config, 0, sizeof(config));
  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,
                                &config, sizeof(config),
                                &config, sizeof(config));    
}


// Get the position geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_position_get_geom(playerc_position_t *device)
{
  int len;
  player_position_geom_t config;

  memset(&config, 0, sizeof(config));
  config.subtype = PLAYER_POSITION_GET_GEOM_REQ;

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
  device->pose[2] = ((int16_t) ntohs(config.pose[2])) * M_PI / 180;
  device->size[0] = ((int16_t) ntohs(config.size[0])) / 1000.0;
  device->size[1] = ((int16_t) ntohs(config.size[1])) / 1000.0;

  return 0;
}


// Set the robot speed
int playerc_position_set_cmd_vel(playerc_position_t *device, double vx, double vy,
                                 double va, int state)
{
  player_position_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.xspeed = htonl((int) (vx * 1000.0));
  cmd.yspeed = htonl((int) (vy * 1000.0));
  cmd.yawspeed = htonl((int) (va * 180.0 / M_PI));
  cmd.state = state;
  cmd.type = 0;

  return playerc_client_write(device->info.client, &device->info, &cmd, sizeof(cmd));
}


// Set the target pose
int playerc_position_set_cmd_pose(playerc_position_t *device, double gx, double gy,
                                  double ga, int state)
{
  player_position_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.xpos = htonl((int) (gx * 1000.0));
  cmd.ypos = htonl((int) (gy * 1000.0));
  cmd.yaw = htonl((int) (ga * 180.0 / M_PI));
  cmd.state = state;
  cmd.type = 1;

  return playerc_client_write(device->info.client, &device->info, &cmd, sizeof(cmd));
}

// Get the list of waypoints.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_position_get_waypoints(playerc_position_t *device)
{
  int i;
  int len;
  player_position_waypoints_req_t config;

  memset(&config, 0, sizeof(config));
  config.subtype = PLAYER_POSITION_GET_WAYPOINTS_REQ;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, 
                               sizeof(config));
  if (len < 0)
    return -1;
  if (len == 0)
  {
    PLAYERC_ERR("got unexpected zero-length reply");
    return -1;
  }
  
  device->gx = ((int)htonl(config.goal[0])) / 1e3;
  device->gy = ((int)htonl(config.goal[1])) / 1e3;
  device->ga = DTOR((int)htonl(config.goal[0]));
  device->path_valid = config.path_valid;

  if(device->path_valid)
  {
    device->waypoint_count = (int)ntohs(config.count);
    for(i=0;i<device->waypoint_count;i++)
    {
      device->waypoints[i][0] = ((int)ntohl(config.waypoints[i].x)) / 1e3;
      device->waypoints[i][1] = ((int)ntohl(config.waypoints[i].y)) / 1e3;
    }
  }
  return 0;
}
