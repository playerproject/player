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
 * Desc: ir proxy
 * Author: Toby Collett (based on ir proxy by Andrew Howard)
 * Date: 13 Feb 2004
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
void playerc_ir_putdata(playerc_ir_t *device, player_msghdr_t *header,
                           player_ir_data_t *data, size_t len);

// Create a new ir proxy
playerc_ir_t *playerc_ir_create(playerc_client_t *client, int index)
{
  playerc_ir_t *device;

  device = malloc(sizeof(playerc_ir_t));
  memset(device, 0, sizeof(playerc_ir_t));
  playerc_device_init(&device->info, client, PLAYER_IR_CODE, index,
                      (playerc_putdata_fn_t) playerc_ir_putdata);
    
  return device;
}


// Destroy a ir proxy
void playerc_ir_destroy(playerc_ir_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the ir device
int playerc_ir_subscribe(playerc_ir_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the ir device
int playerc_ir_unsubscribe(playerc_ir_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_ir_putdata(playerc_ir_t *device, player_msghdr_t *header,
                           player_ir_data_t *data, size_t len)
{
  int i;

  assert(sizeof(*data) <= len);

  device->ranges.range_count = ntohs(data->range_count);

  // copy data into packet
  for (i = 0; i < device->ranges.range_count; i++)
  { 
    device->ranges.voltages[i] = ntohs(data->voltages[i]);
    device->ranges.ranges[i] = ntohs(data->ranges[i]);
  }
}


// Get the ir geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_ir_get_geom(playerc_ir_t *device)
{
  int i, len;
  player_ir_pose_req_t config;

  config.subtype = PLAYER_IR_POSE_REQ;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  device->poses.pose_count = htons(config.poses.pose_count);
  for (i = 0; i < device->poses.pose_count; i++)
  {
    device->poses.poses[i][0] = ((int16_t) ntohs(config.poses.poses[i][0])); //mm
    device->poses.poses[i][1] = ((int16_t) ntohs(config.poses.poses[i][1])); //mm
    device->poses.poses[i][2] = ((int16_t) ntohs(config.poses.poses[i][2])); //deg
  }

  return 0;
}


