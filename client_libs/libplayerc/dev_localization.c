/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002
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
/***************************************************************************
 * Desc: localize device proxy
 * Author: Boyoon Jung
 * Date: 20 Jun 2002
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
void playerc_localize_putdata(playerc_localize_t *device, player_msghdr_t *header,
                           player_localize_data_t *data, size_t len);

// Create a new localize proxy
playerc_localize_t *playerc_localize_create(playerc_client_t *client, int robot, int index)
{
  playerc_localize_t *device;

  device = malloc(sizeof(playerc_localize_t));
  memset(device, 0, sizeof(playerc_localize_t));
  playerc_device_init(&device->info, client, robot, PLAYER_LOCALIZE_CODE, index,
                      (playerc_putdata_fn_t) playerc_localize_putdata);
    
  return device;
}


// Destroy a localize proxy
void playerc_localize_destroy(playerc_localize_t *device)
{
  if (device->map_cells)
    free(device->map_cells);
  playerc_device_term(&device->info);
  free(device);
  return;
}


// Subscribe to the localize device
int playerc_localize_subscribe(playerc_localize_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the localize device
int playerc_localize_unsubscribe(playerc_localize_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_localize_putdata(playerc_localize_t *device, player_msghdr_t *header,
                              player_localize_data_t *data, size_t len)
{
  int i, j, k;

  assert(len >= sizeof(*data) - sizeof(data->hypoths));
  
  // Byte-swap the data
  data->hypoth_count = ntohl(data->hypoth_count);
  for (i = 0; i < data->hypoth_count; i++)
  {
    for (j = 0; j < 3; j++)
      data->hypoths[i].mean[j] = ntohl(data->hypoths[i].mean[j]);
    for (j = 0; j < 3; j++)
      for (k = 0; k < 3; k++)
        data->hypoths[i].cov[j][k] = ntohl(data->hypoths[i].cov[j][k]);
    data->hypoths[i].alpha = ntohl(data->hypoths[i].alpha);
  }

  // Read the data; unit conversion
  for (i = 0; i < data->hypoth_count; i++)
  {
    // Linear components
    for (j = 0; j < 2; j++)
    {
      device->hypoths[i].mean[j] = data->hypoths[i].mean[j] / 1000.0;      
      for (k = 0; k < 2; k++)
        device->hypoths[i].cov[j][k] = data->hypoths[i].cov[j][k] / (1000.0 * 1000.0);
    }

    // Angular components
    device->hypoths[i].mean[2] = data->hypoths[i].mean[2] * M_PI / (180 * 3600);
    device->hypoths[i].cov[2][2] = data->hypoths[i].cov[2][2] / (M_PI / (180 * 3600) * M_PI / (180 * 3600));

    // Weights
    device->hypoths[i].weight = data->hypoths[i].alpha / (1e6);
  }
  device->hypoth_count = data->hypoth_count;

  return;
}


// Reset the localize device
int playerc_localize_reset(playerc_localize_t *device)
{
  int len;
  player_localize_reset_t reset;

  reset.subtype = PLAYER_LOCALIZE_RESET_REQ;

  len = playerc_client_request(device->info.client, &device->info,
                               &reset, sizeof(reset), &reset, sizeof(reset));

  if (len < 0)
    return -1;

  // TODO: check for NACK

  return 0;
}


// Retrieve the occupancy map.  The map is written into the proxy
// structure.
int playerc_localize_get_map(playerc_localize_t *device)
{
  int i, j;
  int ni, nj;
  int sx, sy;
  int si, sj;
  int len;
  size_t reqlen;
  player_localize_map_info_t info;
  player_localize_map_data_t data;
  
  // Get the map header
  info.subtype = PLAYER_LOCALIZE_GET_MAP_INFO_REQ;
  reqlen = sizeof(info.subtype);
  len = playerc_client_request(device->info.client, &device->info,
                               &info, reqlen, &info, sizeof(info));
  if (len < 0)
    return -1;
  if (len != sizeof(info))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(info));
    return -1;
  }

  device->map_size_x = ntohl(info.width);
  device->map_size_y = ntohl(info.height);
  device->map_scale = 1000.0 / ((double) (int32_t) ntohl(info.scale));

  // TESTING
  printf("map info %d %d %f\n", device->map_size_x, device->map_size_y, device->map_scale);

  if (device->map_cells)
    free(device->map_cells);
  device->map_cells = malloc(device->map_size_x * device->map_size_y * sizeof(device->map_cells[0]));

  // Tile size
  sx = (int) sqrt(sizeof(data.data));
  sy = sx;
  assert(sx * sy < sizeof(data.data));

  // Get the map data in tiles
  for (j = 0; j < device->map_size_y; j += sy)
  {
    for (i = 0; i < device->map_size_x; i += sx)
    {
      si = MIN(sx, device->map_size_x - i);
      sj = MIN(sy, device->map_size_y - j);
      
      data.subtype = PLAYER_LOCALIZE_GET_MAP_DATA_REQ;
      data.col = htonl(i);
      data.row = htonl(j);
      data.width = htonl(si);
      data.height = htonl(sj); 
      reqlen = sizeof(data) - sizeof(data.data);
    
      len = playerc_client_request(device->info.client, &device->info,
                                   &data, reqlen, &data, sizeof(data));
      if (len < 0)
        return -1;
      if (len < reqlen + si * sj)
      {
        PLAYERC_ERR2("reply has unexpected length (%d < %d)", len, reqlen + si * sj);
        return -1;
      }

      for (nj = 0; nj < sj; nj++)
        for (ni = 0; ni < si; ni++)
          device->map_cells[(i + ni) + (j + nj) * device->map_size_x] = data.data[ni + nj * si];
    }
  }
  
  return 0;
}


// Get the current configuration.
int playerc_localize_get_config(playerc_localize_t *device,
                                    player_localize_config_t *cfg)
{
  int len;
  player_localize_config_t config;

  config.subtype = PLAYER_LOCALIZE_GET_CONFIG_REQ;
    
  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config.subtype), &config, sizeof(config));

  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(config));
    return -1;
  }

  // fill out the data field
  cfg->subtype = PLAYER_LOCALIZE_GET_CONFIG_REQ;
  cfg->num_particles = (uint32_t) ntohl(config.num_particles);

  return 0;
}


// Modify the current configuration.
int playerc_localize_set_config(playerc_localize_t *device,
                                    player_localize_config_t cfg)
{
  int len;
  player_localize_config_t config;

  config.subtype = PLAYER_LOCALIZE_SET_CONFIG_REQ;
  config.num_particles = cfg.num_particles;

  len = playerc_client_request(device->info.client, &device->info,
                               &config, sizeof(config), &config, sizeof(config));

  if (len < 0)
    return -1;

  // TODO: check for NACK

  return 0;
}


/* REMOVE
   
// Retrieve the information of the internal grid map
int playerc_localize_get_map_header(playerc_localize_t *device,
                                        uint8_t scale, player_localize_map_header_t* header)
{
  int len;
  player_localize_map_header_t map_header;

  map_header.subtype = PLAYER_LOCALIZE_GET_MAP_HDR_REQ;
  map_header.scale = scale;
    
  len = playerc_client_request(device->info.client, &device->info,
                               &map_header, sizeof(map_header.subtype)+sizeof(map_header.scale),
                               &map_header, sizeof(map_header));
  if (len < 0)
    return -1;
  if (len != sizeof(map_header))
  {
    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len,
                 sizeof(map_header.subtype)+sizeof(map_header.scale));
    return -1;
  }

  header->subtype = map_header.subtype;
  header->scale = map_header.scale;
  header->width = (uint32_t) ntohl(map_header.width);
  header->height = (uint32_t) ntohl(map_header.height);
  header->ppkm = (uint32_t) ntohl(map_header.ppkm);

  return 0;
}


// Retrieve the scaled grid map
int playerc_localize_get_map(playerc_localize_t *device, int8_t scale,
                                 player_localize_map_header_t* header, char *data)
{
  int len, r, count;
  player_localize_map_data_t row_data;

  // retrieve the map header
  len = playerc_localize_get_map_header(device, scale, header);
  if (len < 0)
    return -1;

  // error check
  if (header->width >= PLAYER_MAX_REQREP_SIZE-4)
    return -1;

  // retrieve a scaled map
  count = sizeof(row_data.data) / header->width;
  for (r=0; r<header->height; r+=count)
  {
    // retrieve a single row data
    row_data.subtype = PLAYER_LOCALIZE_GET_MAP_DATA_REQ;
    row_data.scale = scale;
    row_data.row = htons(r);
    len = playerc_client_request(device->info.client, &device->info,
                                 &row_data, sizeof(row_data.subtype) + sizeof(row_data.scale) +
                                 sizeof(row_data.row), &row_data, sizeof(row_data));
    if (len < 0)
	    return -1;
    if (len != sizeof(row_data))
    {
	    PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(row_data));
	    return -1;
    }

    // fill out the data buffer
    if (r + count >= header->height)
	    len = header->width * (header->height - r);
    else
	    len = header->width * count;
    memcpy(data+(r*header->width), row_data.data, len);
  }

  return 0;
}
*/
