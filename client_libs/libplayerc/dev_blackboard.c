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
 * Desc: Blackboard device proxy
 * Author: Benjamin Morelli
 * Date: September 2007
 * CVS: $Id$
 **************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "playerc.h"
#include "error.h"
#include <libplayerxdr/playerxdr.h>

void playerc_blackboard_putmsg(playerc_blackboard_t *device,
                               player_msghdr_t *header,
                               player_blackboard_entry_t *data, size_t len);

// Create a new blackboard proxy
playerc_blackboard_t *playerc_blackboard_create(playerc_client_t *client, int index)
{
	playerc_blackboard_t *device= NULL;
	device = malloc(sizeof(playerc_blackboard_t));
	memset(device, 0, sizeof(playerc_blackboard_t));

	playerc_device_init(&device->info, client, PLAYER_BLACKBOARD_CODE, index, (playerc_putmsg_fn_t)playerc_blackboard_putmsg);

	return device;
}

// Destroy a blackboard proxy
void playerc_blackboard_destroy(playerc_blackboard_t *device)
{
	playerc_device_term(&device->info);
	//playerc_blackboard_cleanup(device);
	free(device);
}

// Subscribe to the blackboard device
int playerc_blackboard_subscribe(playerc_blackboard_t *device, int access)
{
	return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the blackboard device
int playerc_blackboard_unsubscribe(playerc_blackboard_t *device)
{
	return playerc_device_unsubscribe(&device->info);
}

// Subscribe to a blackboard key
int playerc_blackboard_subscribe_to_key(playerc_blackboard_t* device, const char* key,
		player_blackboard_entry_t* entry_out)
{
	player_blackboard_entry_t req;
	memset(&req, 0, sizeof(req));
	req.key = strdup(key);
	req.key_count = strlen(key) + 1;

	if (playerc_client_request(
		device->info.client,
		&device->info,
		PLAYER_BLACKBOARD_REQ_SUBSCRIBE_TO_KEY,
		&req, NULL) < 0)
	{
		free(req.key);
		PLAYERC_ERR("failed to subscribe to blackboard key");
		return -1;
	}

	free(req.key);
	return 0;
}

// Unsubscribe from a blackboard key
int playerc_blackboard_unsubscribe_from_key(playerc_blackboard_t* device, const char* key)
{
	player_blackboard_entry_t req;
	memset(&req, 0, sizeof(req));
	req.key = strdup(key);
	req.key_count = strlen(key) + 1;

	if (playerc_client_request(
		device->info.client,
		&device->info,
		PLAYER_BLACKBOARD_REQ_UNSUBSCRIBE_FROM_KEY,
		&req,
		NULL) < 0)
	{
		free(req.key);
		PLAYERC_ERR("failed to unsubscribe to blackboard key");
		return -1;
	}

	free(req.key);
	return 0;

}

// Set a key
int playerc_blackboard_set_entry(playerc_blackboard_t *device, player_blackboard_entry_t* entry)
{
	if (playerc_client_request(
		device->info.client,
		&device->info,
		PLAYER_BLACKBOARD_REQ_SET_ENTRY,
		entry, NULL) < 0)
	{
		PLAYERC_ERR("failed to set blackboard key");
		return -1;
	}

	return 0;
}

void playerc_blackboard_putmsg(playerc_blackboard_t *device,
                               player_msghdr_t *header,
                               player_blackboard_entry_t *data, size_t len)
{
	if (device->on_blackboard_event != NULL)
	{
		device->on_blackboard_event(*data);
	}
}
