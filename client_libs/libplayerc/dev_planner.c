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
 * Desc: Planner device proxy
 * Author: Brian Gerkey
 * Date: June 2004
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
void playerc_planner_putdata(playerc_planner_t *device, player_msghdr_t *header,
                              player_planner_data_t *data, size_t len);


// Create a new planner proxy
playerc_planner_t *playerc_planner_create(playerc_client_t *client, int index)
{
  playerc_planner_t *device;

  device = malloc(sizeof(playerc_planner_t));
  memset(device, 0, sizeof(playerc_planner_t));
  playerc_device_init(&device->info, client, PLAYER_PLANNER_CODE, index,
                      (playerc_putdata_fn_t) playerc_planner_putdata);

  
  return device;
}


// Destroy a planner proxy
void playerc_planner_destroy(playerc_planner_t *device)
{
  playerc_device_term(&device->info);
  free(device);

  return;
}


// Subscribe to the planner device
int playerc_planner_subscribe(playerc_planner_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the planner device
int playerc_planner_unsubscribe(playerc_planner_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_planner_putdata(playerc_planner_t *device, player_msghdr_t *header,
                              player_planner_data_t *data, size_t len)
{
  device->path_valid = data->valid;
  device->path_done = data->done;

  device->px = (long) ntohl(data->px) / 1000.0;
  device->py = (long) ntohl(data->py) / 1000.0;
  device->pa = (long) ntohl(data->pa) * M_PI / 180.0;
  device->pa = atan2(sin(device->pa), cos(device->pa));

  device->gx = (long) ntohl(data->gx) / 1000.0;
  device->gy = (long) ntohl(data->gy) / 1000.0;
  device->ga = (long) ntohl(data->ga) * M_PI / 180.0;
  device->ga = atan2(sin(device->ga), cos(device->ga));

  device->wx = (long) ntohl(data->wx) / 1000.0;
  device->wy = (long) ntohl(data->wy) / 1000.0;
  device->wa = (long) ntohl(data->wa) * M_PI / 180.0;
  device->wa = atan2(sin(device->wa), cos(device->wa));

  device->curr_waypoint = (int)ntohs(data->curr_waypoint);
  device->waypoint_count = (unsigned int)ntohs(data->waypoint_count);
}


int playerc_planner_set_cmd_pose(playerc_planner_t *device, double gx, double gy,
                                  double ga, int state)
{
  player_planner_cmd_t cmd;

  memset(&cmd, 0, sizeof(cmd));
  cmd.gx = htonl((int) (gx * 1000.0));
  cmd.gy = htonl((int) (gy * 1000.0));
  cmd.ga = htonl((int) (ga * 180.0 / M_PI));

  return playerc_client_write(device->info.client, &device->info, &cmd, sizeof(cmd));
}

// Get the list of waypoints.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_planner_get_waypoints(playerc_planner_t *device)
{
  int i;
  int len;
  player_planner_waypoints_req_t config;

  memset(&config, 0, sizeof(config));
  config.subtype = PLAYER_PLANNER_GET_WAYPOINTS_REQ;

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
  
  device->waypoint_count = (int)ntohs(config.count);
  for(i=0;i<device->waypoint_count;i++)
  {
    device->waypoints[i][0] = ((int)ntohl(config.waypoints[i].x)) / 1e3;
    device->waypoints[i][1] = ((int)ntohl(config.waypoints[i].y)) / 1e3;
    device->waypoints[i][2] = DTOR((int)ntohl(config.waypoints[i].a));
  }
  return 0;
}
