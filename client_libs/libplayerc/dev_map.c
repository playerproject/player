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
 * Author: Brian gerkey
 * Date: June 2004
 * CVS: $Id$
 **************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "playerc.h"
#include "error.h"


// Create a new map proxy
playerc_map_t *playerc_map_create(playerc_client_t *client, int index)
{
  playerc_map_t *device;

  device = malloc(sizeof(playerc_map_t));
  memset(device, 0, sizeof(playerc_map_t));
  playerc_device_init(&device->info, client, PLAYER_MAP_CODE, index,
                      (playerc_putmsg_fn_t) NULL);
    
  return device;
}


// Destroy a map proxy
void playerc_map_destroy(playerc_map_t *device)
{
  playerc_device_term(&device->info);
  if(device->cells)
    free(device->cells);
  free(device);
}

// Subscribe to the map device
int playerc_map_subscribe(playerc_map_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}

// Un-subscribe from the map device
int playerc_map_unsubscribe(playerc_map_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}

int playerc_map_get_map(playerc_map_t* device)
{
  player_map_info_t info_req;
  player_map_data_t* data_req;
  size_t repsize;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;
  char* cell;

  // first, get the map info
  if(playerc_client_request(device->info.client, 
                            &device->info, 
                            PLAYER_MAP_REQ_GET_INFO, 
                            NULL, &info_req, sizeof(info_req)) < 0)
  {
    PLAYERC_ERR("failed to get map info");
    return(-1);
  }

  device->resolution = info_req.scale;
  device->width = info_req.width;
  device->height = info_req.height;
  device->origin[0] = info_req.origin.px;
  device->origin[1] = info_req.origin.py;

  // Allocate space for the whole map
  if(device->cells)
    free(device->cells);
  device->cells = (char*)malloc(sizeof(char) *
                                device->width * device->height);
  assert(device->cells);

  // now, get the map, in tiles

  // Allocate space for one received tile.  Note that we need to allocate
  // space to hold the maximum possible tile, because the received tile
  // will be unpacked into a structure of that size.
  repsize = sizeof(player_map_data_t);
  data_req = (player_map_data_t*)malloc(repsize);
  assert(data_req);

  // Tile size
  sy = sx = (int)sqrt(PLAYER_MAP_MAX_TILE_SIZE);
  assert(sx * sy < (int)(PLAYER_MAP_MAX_TILE_SIZE));
  oi=oj=0;
  while((oi < device->width) && (oj < device->height))
  {
    si = MIN(sx, device->width - oi);
    sj = MIN(sy, device->height - oj);

    memset(data_req,0,repsize);
    data_req->col = oi;
    data_req->row = oj;
    data_req->width = si;
    data_req->height = sj;

    if(playerc_client_request(device->info.client, &device->info,
                              PLAYER_MAP_REQ_GET_DATA,
                              data_req, data_req, repsize) < 0)
    {
      PLAYERC_ERR("failed to get map data");
      free(data_req);
      free(device->cells);
      return(-1);
    }

    // copy the map data
    for(j=0;j<sj;j++)
    {
      for(i=0;i<si;i++)
      {
        cell = device->cells + PLAYERC_MAP_INDEX(device,oi+i,oj+j);
        *cell = data_req->data[j*si + i];
      }
    }

    oi += si;
    if(oi >= device->width)
    {
      oi = 0;
      oj += sj;
    }
  }

  free(data_req);

  return(0);
}
