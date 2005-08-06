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
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"


// Create a new map proxy
playerc_map_t *playerc_map_create(playerc_client_t *client, int index)
{
  playerc_map_t *device;

  device = malloc(sizeof(playerc_map_t));
  memset(device, 0, sizeof(playerc_map_t));
  playerc_device_init(&device->info, client, PLAYER_MAP_CODE, index,
                      (playerc_putdata_fn_t) playerc_map_putdata);
    
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

// Process incoming data
void playerc_map_putdata(playerc_map_t *device, player_msghdr_t *header,
                         player_map_data_t *data, size_t len)
{
  int i,j;
  char* cell;

  data->col = ntohl(data->col);
  data->row = ntohl(data->row);
  data->width = ntohl(data->width);
  data->height = ntohl(data->height);

  // copy the map data
  for(j=0;j<data->height;j++)
  {
    for(i=0;i<data->width;i++)
    {
      cell = device->cells + PLAYERC_MAP_INDEX(device,
                                               data->col+i,
                                               data->row+j);
      *cell = data->data[j*data->width + i];
    }
  }
}

int playerc_map_get_map(playerc_map_t* device)
{
  player_map_info_t info_req;
  player_map_data_t data_req;
  int reqlen, replen;
  int i,j;
  int oi,oj;
  int sx,sy;
  int si,sj;
  char* cell;

  // first, get the map info
  memset(&info_req, 0, sizeof(info_req));
  info_req.subtype = PLAYER_MAP_GET_INFO_REQ;

  if(playerc_client_request(device->info.client, &device->info,
                            &info_req, sizeof(info_req.subtype),
                            &info_req, sizeof(info_req)) < 0)
  {
    PLAYERC_ERR("failed to get map info");
    return(-1);
  }

  device->resolution = 1/(ntohl(info_req.scale) / 1e3);
  device->width = ntohl(info_req.width);
  device->height = ntohl(info_req.height);

  // allocate space for cells
  if(device->cells)
    free(device->cells);
  assert(device->cells = (char*)malloc(sizeof(char) *
                                       device->width * device->height));

  // now, get the map, in tiles
  data_req.subtype = PLAYER_MAP_GET_DATA_REQ;

  // Tile size
  sy = sx = (int)sqrt(sizeof(data_req.data));
  assert(sx * sy < (int)sizeof(data_req.data));
  oi=oj=0;
  while((oi < device->width) && (oj < device->height))
  {
    si = MIN(sx, device->width - oi);
    sj = MIN(sy, device->height - oj);

    data_req.col = htonl(oi);
    data_req.row = htonl(oj);
    data_req.width = htonl(si);
    data_req.height = htonl(sj);

    reqlen = sizeof(data_req) - sizeof(data_req.data);

    if((replen = playerc_client_request(device->info.client, &device->info,
                                        &data_req, reqlen,
                                        &data_req, sizeof(data_req))) == 0)
    {
      PLAYERC_ERR("failed to get map info");
      return(-1);
    }
    else if(replen != (reqlen + si * sj))
    {
      PLAYERC_ERR2("got less map data than expected (%d != %d)",
                    replen, reqlen + si*sj);
      return(-1);
    }

    // copy the map data
    for(j=0;j<sj;j++)
    {
      for(i=0;i<si;i++)
      {
        cell = device->cells + PLAYERC_MAP_INDEX(device,oi+i,oj+j);
        *cell = data_req.data[j*si + i];
      }
    }

    oi += si;
    if(oi >= device->width)
    {
      oi = 0;
      oj += sj;
    }
  }

  return(0);
}
