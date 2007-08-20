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
 * Desc: Map device proxy
 * Author: Benjamin Morelli
 * Date: July 2007
 * CVS: $Id$
 **************************************************************************/

#if HAVE_CONFIG_H
#include <config.h>
#endif

/*#if HAVE_ZLIB_H
#include <zlib.h>
#endif*/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "playerc.h"
#include "error.h"
#include <libplayerxdr/playerxdr.h>

// Create a new vectormap proxy
playerc_vectormap_t *playerc_vectormap_create(playerc_client_t *client, int index)
{
  playerc_vectormap_t *device = NULL;
  device = malloc(sizeof(playerc_vectormap_t));
  memset(device, 0, sizeof(playerc_vectormap_t));

  playerc_device_init(&device->info, client, PLAYER_VECTORMAP_CODE, index,
                       (playerc_putmsg_fn_t) NULL);

  return device;
}


// Destroy a vectormap proxy
void playerc_vectormap_destroy(playerc_vectormap_t *device)
{
  playerc_device_term(&device->info);
  playerc_vectormap_cleanup(device);
  free(device);
}

// Subscribe to the vectormap device
int playerc_vectormap_subscribe(playerc_vectormap_t *device, int access)
{
#ifdef HAVE_GEOS
  initGEOS(printf,printf);
#endif
  return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the vectormap device
int playerc_vectormap_unsubscribe(playerc_vectormap_t *device)
{
#ifdef HAVE_GEOS
  finishGEOS();
#endif
  return playerc_device_unsubscribe(&device->info);
}

// Get vectormap meta-data
int playerc_vectormap_get_map_info(playerc_vectormap_t* device)
{
  int ii;
  player_vectormap_info_t info_req;
  memset(&info_req, 0, sizeof(info_req));

  // try to get map info
  if (playerc_client_request(
      device->info.client,
      &device->info,
      PLAYER_VECTORMAP_REQ_GET_MAP_INFO,
      NULL,
      &info_req,
      sizeof(player_vectormap_info_t)) < 0)
  {
    PLAYERC_ERR("failed to get vectormap info");
    return -1;
  }

  playerc_vectormap_cleanup(device);
  device->srid = info_req.srid;
  device->extent = info_req.extent;
  device->layers_count = info_req.layers_count;

  // allocate mem for array of layers and memset to 0
  device->layers = calloc(device->layers_count, sizeof(player_vectormap_layer_data_t*));
  if (!device->layers)
  {
    PLAYERC_ERR("calloc failed. failed to get vectormap info");
    return -1;
  }

  for (ii=0; ii<device->layers_count; ++ii)
  {
    device->layers[ii] = malloc(sizeof(player_vectormap_layer_data_t));
    device->layers[ii]->info = info_req.layers[ii].info;
  }

  return 0;
}

// get layer info
int playerc_vectormap_get_layer_info(playerc_vectormap_t *device, unsigned layer_index)
{
  player_vectormap_layer_info_t info_req = device->layers[layer_index]->info;

  if (playerc_client_request(
      device->info.client,
      &device->info,
      PLAYER_VECTORMAP_REQ_GET_LAYER_INFO,
      &info_req,
      &info_req,
      sizeof(info_req)) < 0)
  {
    PLAYERC_ERR("failed to get layer info");
    return -1;
  }

  device->layers[layer_index]->info = info_req;
  return 0;
}

// Get layer data
int playerc_vectormap_get_layer_data(playerc_vectormap_t *device, unsigned layer_index)
{
  player_vectormap_layer_data_t data_req;
  memset(&data_req, 0, sizeof(data_req));
  player_vectormap_layer_info_t layer_info = device->layers[layer_index]->info;

  data_req.info = layer_info;

  if (playerc_client_request(
      device->info.client,
      &device->info,
      PLAYER_VECTORMAP_REQ_GET_LAYER_DATA,
      &data_req,
      &data_req,
      sizeof(data_req)) < 0)
  {
    PLAYERC_ERR("failed to get layer data");
    return -1;
  }

  memset(device->layers[layer_index], 0, sizeof(player_vectormap_layer_data_t));
  data_req.info = layer_info;
  memcpy(device->layers[layer_index], &data_req, sizeof(data_req));

  return 0;
}

GEOSGeom playerc_vectormap_get_feature_data(playerc_vectormap_t *device, unsigned layer_index, unsigned feature_index)
{
  int i;
#ifdef HAVE_GEOS
  /*printf("%p %d\n", device->layers[layer_index]->features[feature_index].wkb, device->layers[layer_index]->features[feature_index].wkb_count);
  for(i = 0; i < device->layers[layer_index]->features[feature_index].wkb_count; i++)
    printf("%02x", device->layers[layer_index]->features[feature_index].wkb[i]);
  printf("\n");*/
  return GEOSGeomFromWKB_buf(
    device->layers[layer_index]->features[feature_index].wkb,
    device->layers[layer_index]->features[feature_index].wkb_count
  );
#else
  return NULL;
#endif
}

void playerc_vectormap_cleanup(playerc_vectormap_t *device)
{
  unsigned ii;
  if (device->layers)
  {
    for (ii=0; ii<device->layers_count; ++ii)
    {
      player_vectormap_layer_data_t_cleanup(device->layers[ii]);
    }
    free(device->layers);
  }
  device->srid = -1;
  device->layers_count = 0;
  memset(&device->extent, 0, sizeof(player_extent2d_t));
}
