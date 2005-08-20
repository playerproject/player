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

#include "playerc.h"
#include "error.h"


// Create a new laser proxy
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index)
{
  playerc_laser_t *device;

  device = malloc(sizeof(playerc_laser_t));
  memset(device, 0, sizeof(playerc_laser_t));
#if 0
  playerc_device_init(&device->info, client, PLAYER_LASER_CODE, index,
                      (playerc_putdata_fn_t) playerc_laser_putdata,
					  (playerc_putdata_fn_t) playerc_laser_putgeom,
					  (playerc_putdata_fn_t) playerc_laser_putconfig);
#endif
  playerc_device_init(&device->info, client, PLAYER_LASER_CODE, index,
                      (playerc_putdata_fn_t) playerc_laser_putdata,
					  (playerc_putdata_fn_t) NULL,
					  (playerc_putdata_fn_t) NULL);

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
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           void *data, size_t len)
{
  int i;
  double r, b, db;
  player_laser_data_t ldata;

  if(player_laser_data_pack(data, len, &ldata, PLAYERXDR_DECODE) < 0)
  {
    PLAYERC_ERR("failed to unpack laser data message");
    return;
  }

  assert(ldata.ranges_count <= sizeof(device->scan) / sizeof(device->scan[0]));
  
  b = ldata.min_angle;
  db = ldata.resolution;

  device->scan_start = b;
  device->scan_res = db;
  
  for (i = 0; i < ldata.ranges_count; i++)
  {
    r = ldata.ranges[i];
    assert(r >= 0);
    device->ranges[i] = r;
    device->scan[i][0] = r;
    device->scan[i][1] = b;
    device->point[i][0] = r * cos(b);
    device->point[i][1] = r * sin(b);
    device->intensity[i] = ldata.intensity[i];
    b += db;
  }

  device->scan_count = ldata.ranges_count;
}

#if 0
// Process incoming config
void playerc_laser_putconfig(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_config_t *data, size_t len)
{
  if (len != sizeof(player_laser_config_t))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(player_laser_config_t));
    return;
  }
  
  device->scan_res = ntohs(data->resolution);
  device->scan_start = (short) ntohs(data->min_angle) / 100.0 * M_PI / 180;
  device->scan_count = (((short) ntohs(data->max_angle) / 100.0 * M_PI / 180) - device->scan_start) / device->scan_res;;
  device->intensity_on = data->intensity;
  device->range_res = (int) ntohs(data->range_res);
  
}

// Process incoming geom
void playerc_laser_putgeom(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_geom_t *data, size_t len)
{
  if (len != sizeof(player_laser_geom_t))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(player_laser_geom_t));
    return;
  }

  device->pose[0] = ((int16_t) ntohs(data->pose[0])) / 1000.0;
  device->pose[1] = ((int16_t) ntohs(data->pose[1])) / 1000.0;
  device->pose[2] = ((int16_t) ntohs(data->pose[2])) * M_PI / 180;
  device->size[0] = ((int16_t) ntohs(data->size[0])) / 1000.0;
  device->size[1] = ((int16_t) ntohs(data->size[1])) / 1000.0;
  
//  printf("Laser Geometry: %f %f %f %f %f\n",device->pose[0],device->pose[1],device->pose[2],device->size[0],device->size[1]);
}

// Configure the laser.
int  playerc_laser_set_config(playerc_laser_t *device, double min_angle, double max_angle,
                              int resolution, int range_res, int intensity)
{
  int len;
  player_laser_config_t config;

//  config.subtype = PLAYER_LASER_SET_CONFIG;
  config.min_angle = htons((unsigned int) (int) (min_angle * 180.0 / M_PI * 100));
  config.max_angle = htons((unsigned int) (int) (max_angle * 180.0 / M_PI * 100));
  config.resolution = htons(resolution);
  config.intensity = (intensity ? 1 : 0);
  config.range_res = htons((uint16_t)range_res);

  len = playerc_client_request(device->info.client, &device->info,PLAYER_LASER_SET_CONFIG,
                               &config, sizeof(config), &config, sizeof(config));
  if (len < 0)
    return -1;

  // TODO: check for NACK
  
  return 0;
}


// Get the laser configuration.
int playerc_laser_get_config(playerc_laser_t *device, double *min_angle, double *max_angle,
                             int *resolution, int *range_res, int *intensity)
{
  player_laser_config_t config;

//  config.subtype = PLAYER_LASER_GET_CONFIG;

  playerc_client_request(device->info.client, &device->info,PLAYER_LASER_GET_CONFIG,
                               &config, 0, &config, sizeof(config));


   while(!device->info.freshconfig)
   {
   		//printf("waiting for laser config message\n");
   		playerc_client_read(device->info.client);
   }


  *min_angle = device->scan_start;
  *max_angle = device->scan_start + device->scan_count * device->scan_res;
  *resolution = device->scan_res;
  *intensity = device->intensity_on;
  *range_res = device->range_res;
  return 0;
}


// Get the laser geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int playerc_laser_get_geom(playerc_laser_t *device)
{
  int len;
  player_laser_geom_t config;

//  config.subtype = PLAYER_LASER_GET_GEOM;

  len = playerc_client_request(device->info.client, &device->info,PLAYER_LASER_GET_GEOM,
                               &config, 0, &config, sizeof(config));
  if (len < sizeof(config))
    return -1;

  device->pose[0] = ((int16_t) ntohs(config.pose[0])) / 1000.0;
  device->pose[1] = ((int16_t) ntohs(config.pose[1])) / 1000.0;
  device->pose[2] = ((int16_t) ntohs(config.pose[2])) * M_PI / 180;
  device->size[0] = ((int16_t) ntohs(config.size[0])) / 1000.0;
  device->size[1] = ((int16_t) ntohs(config.size[1])) / 1000.0;
  
  return 0;
}

#endif

