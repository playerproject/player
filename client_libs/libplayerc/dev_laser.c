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
 * Desc: Scanning range finder (SRF) proxy
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

#include <libplayercore/playercommon.h>
#include "playerc.h"
#include "error.h"

// Process incoming data
void playerc_laser_putmsg(playerc_laser_t *device, 
                          player_msghdr_t *header,
                          void *data);

// Create a new laser proxy
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index)
{
  playerc_laser_t *device;

  device = malloc(sizeof(playerc_laser_t));
  memset(device, 0, sizeof(playerc_laser_t));
  playerc_device_init(&device->info, client, PLAYER_LASER_CODE, index,
                      (playerc_putmsg_fn_t) playerc_laser_putmsg);

  device->pose[0] = 0.0;
  device->pose[1] = 0.0;
  device->pose[2] = 0.0;
  device->size[0] = 0.15;
  device->size[1] = 0.15;

  return device;
}


// Destroy a laser proxy
void playerc_laser_destroy(playerc_laser_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the laser device
int playerc_laser_subscribe(playerc_laser_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the laser device
int playerc_laser_unsubscribe(playerc_laser_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_laser_putmsg(playerc_laser_t *device, 
                          player_msghdr_t *header,
                          void *data)
{
  int i;
  double r, b, db;
  player_laser_data_t* scan_data;

  if((header->type == PLAYER_MSGTYPE_DATA) &&
     (header->subtype == PLAYER_LASER_DATA_SCAN))
  {
    scan_data = (player_laser_data_t*)data;
    assert(scan_data->ranges_count <= sizeof(device->scan) / sizeof(device->scan[0]));

    b = scan_data->min_angle;
    db = scan_data->resolution;

    device->scan_start = b;
    device->scan_res = db;

    for (i = 0; i < scan_data->ranges_count; i++)
    {
      r = scan_data->ranges[i];
      assert(r >= 0);
      device->ranges[i] = r;
      device->scan[i][0] = r;
      device->scan[i][1] = b;
      device->point[i][0] = r * cos(b);
      device->point[i][1] = r * sin(b);
      device->intensity[i] = scan_data->intensity[i];
      b += db;
    }

    device->scan_count = scan_data->ranges_count;
    device->scan_id = scan_data->id;
  }
  else
    PLAYERC_WARN2("skipping laser message with unknown type/subtype: %d/%d\n",
                 header->type, header->subtype);
}


// Configure the laser.
int
playerc_laser_set_config(playerc_laser_t *device, 
                         double min_angle, 
                         double max_angle,
                         unsigned char resolution, 
                         unsigned char range_res, 
                         unsigned char intensity)
{
  player_laser_config_t config;

  config.min_angle = min_angle;
  config.max_angle = max_angle;
  config.resolution = resolution;
  config.intensity = (intensity ? 1 : 0);
  config.range_res = range_res;

  if(playerc_client_request(device->info.client, &device->info,
                            PLAYER_LASER_REQ_SET_CONFIG,
                            (void*)&config, &config, sizeof(config)) < 0)
    return -1;

  // copy them locally
  device->scan_start = config.min_angle;
  device->scan_res = DTOR(config.resolution / 1e2);
  device->range_res = config.range_res / 1e3;
  device->intensity_on = config.intensity;

  return 0;
}

// Get the laser configuration.
int
playerc_laser_get_config(playerc_laser_t *device, 
                         double *min_angle, 
                         double *max_angle,
                         unsigned char *resolution, 
                         unsigned char *range_res, 
                         unsigned char *intensity)
{
  player_laser_config_t config;

  if(playerc_client_request(device->info.client, &device->info,
                            PLAYER_LASER_REQ_GET_CONFIG,
                            NULL, &config, sizeof(config)) < 0)
    return(-1);

  *min_angle = device->scan_start = config.min_angle;
  *max_angle = config.max_angle;
  *resolution = config.resolution;
  device->scan_res = DTOR(config.resolution / 1e2);
  *intensity = device->intensity_on = config.intensity;
  *range_res = config.range_res;
  device->range_res = config.range_res / 1e3;
  return 0;
}

// Get the laser geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int
playerc_laser_get_geom(playerc_laser_t *device)
{
  player_laser_geom_t config;

  if(playerc_client_request(device->info.client, 
                            &device->info,PLAYER_LASER_REQ_GET_GEOM,
                            NULL, &config, sizeof(config)) < 0)
    return -1;

  device->pose[0] = config.pose.px;
  device->pose[1] = config.pose.py;
  device->pose[2] = config.pose.pa;
  device->size[0] = config.size.sl;
  device->size[1] = config.size.sw;
  
  return 0;
}

