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
 * Desc: localization device proxy
 * Author: Boyoon Jung
 * Date: 20 Jun 2002
 * CVS: $Id$
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Local declarations
void playerc_localization_putdata(playerc_localization_t *device, player_msghdr_t *header,
                           player_localization_data_t *data, size_t len);

// Create a new localization proxy
playerc_localization_t *playerc_localization_create(playerc_client_t *client, int index)
{
  playerc_localization_t *device;

  device = malloc(sizeof(playerc_localization_t));
  memset(device, 0, sizeof(playerc_localization_t));
  playerc_device_init(&device->info, client, PLAYER_LOCALIZATION_CODE, index,
                      (playerc_putdata_fn_t) playerc_localization_putdata);
    
  return device;
}


// Destroy a localization proxy
void playerc_localization_destroy(playerc_localization_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the localization device
int playerc_localization_subscribe(playerc_localization_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the localization device
int playerc_localization_unsubscribe(playerc_localization_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_localization_putdata(playerc_localization_t *device, player_msghdr_t *header,
                           player_localization_data_t *data, size_t len)
{
    int i;

    assert(sizeof(*data) <= len);

    device->num_hypothesis = (uint32_t) ntohl(data->num_hypothesis);
    for (i = 0; i < device->num_hypothesis; i++) {
	device->hypothesis[i].mean[0] = (int32_t) ntohl(data->hypothesis[i].mean[0]);
	device->hypothesis[i].mean[1] = (int32_t) ntohl(data->hypothesis[i].mean[1]);
	device->hypothesis[i].mean[2] = (int32_t) ntohl(data->hypothesis[i].mean[2]);
	device->hypothesis[i].cov[0][0] = (int32_t) ntohl(data->hypothesis[i].cov[0][0]);
	device->hypothesis[i].cov[0][1] = (int32_t) ntohl(data->hypothesis[i].cov[0][1]);
	device->hypothesis[i].cov[0][2] = (int32_t) ntohl(data->hypothesis[i].cov[0][2]);
	device->hypothesis[i].cov[1][0] = (int32_t) ntohl(data->hypothesis[i].cov[1][0]);
	device->hypothesis[i].cov[1][1] = (int32_t) ntohl(data->hypothesis[i].cov[1][1]);
	device->hypothesis[i].cov[1][2] = (int32_t) ntohl(data->hypothesis[i].cov[1][2]);
	device->hypothesis[i].cov[2][0] = (int32_t) ntohl(data->hypothesis[i].cov[2][0]);
	device->hypothesis[i].cov[2][1] = (int32_t) ntohl(data->hypothesis[i].cov[2][1]);
	device->hypothesis[i].cov[2][2] = (int32_t) ntohl(data->hypothesis[i].cov[2][2]);
	device->hypothesis[i].alpha = (uint32_t) ntohl(data->hypothesis[i].alpha);
    }
}


// Reset the localization device
int playerc_localization_reset(playerc_localization_t *device)
{
    int len;
    player_localization_reset_t reset;

    reset.subtype = PLAYER_LOCALIZATION_RESET_REQ;

    len = playerc_client_request(device->info.client, &device->info,
	    			 &reset, sizeof(reset), &reset, sizeof(reset));

    if (len < 0)
	return -1;

    // TODO: check for NACK

    return 0;
}


// Get the current configuration.
int playerc_localization_get_config(playerc_localization_t *device,
	player_localization_config_t *cfg)
{
    int len;
    player_localization_config_t config;

    config.subtype = PLAYER_LOCALIZATION_GET_CONFIG_REQ;
    
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
    cfg->subtype = PLAYER_LOCALIZATION_GET_CONFIG_REQ;
    cfg->num_particles = (uint32_t) ntohl(config.num_particles);

    return 0;
}


// Modify the current configuration.
int playerc_localization_set_config(playerc_localization_t *device,
	player_localization_config_t cfg)
{
    int len;
    player_localization_config_t config;

    config.subtype = PLAYER_LOCALIZATION_SET_CONFIG_REQ;
    config.num_particles = cfg.num_particles;

    len = playerc_client_request(device->info.client, &device->info,
	    			 &config, sizeof(config), &config, sizeof(config));

    if (len < 0)
	return -1;

    // TODO: check for NACK

    return 0;
}


// Retrieve the information of the internal grid map
int playerc_localization_get_map_header(playerc_localization_t *device,
	uint8_t scale, player_localization_map_header_t* header)
{
    int len;
    player_localization_map_header_t map_header;

    map_header.subtype = PLAYER_LOCALIZATION_GET_MAP_HDR_REQ;
    map_header.scale = scale;
    
    len = playerc_client_request(device->info.client, &device->info,
	    			 &map_header, sizeof(map_header), &map_header, sizeof(map_header));
    if (len < 0)
	return -1;
    if (len != sizeof(map_header))
    {
	PLAYERC_ERR2("reply has unexpected length (%d != %d)", len, sizeof(map_header));
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
int playerc_localization_get_map(playerc_localization_t *device,
	uint8_t scale, player_localization_map_header_t* header, char*data)
{
    int len, r, count;
    player_localization_map_data_t row_data;

    // retrieve the map header
    len = playerc_localization_get_map_header(device, scale, header);
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
	row_data.subtype = PLAYER_LOCALIZATION_GET_MAP_DATA_REQ;
	row_data.scale = scale;
	row_data.row = htons(r);
	len = playerc_client_request(device->info.client, &device->info,
				     &row_data, sizeof(row_data), &row_data, sizeof(row_data));
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
