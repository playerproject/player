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
 * Desc: Truth device proxy (works with Stage).
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 **************************************************************************/
#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "playerc.h"
#include "playerpacket.h"
#include "error.h"

// Local declarations
void playerc_camera_putdata(playerc_camera_t *device, player_msghdr_t *header,
                           player_camera_data_t *data, size_t len);


// Create a new camera proxy
playerc_camera_t *playerc_camera_create(playerc_client_t *client, int index)
{
  playerc_camera_t *device;

  device = malloc(sizeof(playerc_camera_t));
  memset(device, 0, sizeof(playerc_camera_t));
  playerc_device_init(&device->info, client, PLAYER_CAMERA_CODE, index, 
                      (playerc_putdata_fn_t) playerc_camera_putdata); 
  return device;
}


// Destroy a camera proxy
void playerc_camera_destroy(playerc_camera_t *device)
{
  playerc_device_term(&device->info);
  free(device);
}


// Subscribe to the camera device
int playerc_camera_subscribe(playerc_camera_t *device, int access)
{
  return playerc_device_subscribe(&device->info, access);
}


// Un-subscribe from the camera device
int playerc_camera_unsubscribe(playerc_camera_t *device)
{
  return playerc_device_unsubscribe(&device->info);
}


// Process incoming data
void playerc_camera_putdata(playerc_camera_t *device, player_msghdr_t *header,
                            player_camera_data_t *data, size_t len)
{
  assert(sizeof(*data) >= len);

  device->width = ntohs(data->width);
  device->height = ntohs(data->height);
  device->bpp = data->bpp;
  device->format = data->format;
  device->fdiv = ntohs(data->fdiv);
  device->compression = data->compression;
  device->image_size = ntohl(data->image_size);

  assert(device->image_size <= sizeof device->image);
  memcpy(device->image, data->image, device->image_size);
  
  return;
}


// Decompress image data
void playerc_camera_decompress(playerc_camera_t *device)
{
#if HAVE_JPEGLIB_H
  int dst_size;
  unsigned char *dst;

  if (device->compression == PLAYER_CAMERA_COMPRESS_RAW)
    return;
  
  // Create a temp buffer
  dst_size = device->width * device->height * device->bpp / 8;
  dst = malloc(dst_size);

  // Decompress into temp buffer
  jpeg_decompress(dst, dst_size, device->image, device->image_size);

  // Copy uncompress image
  device->image_size = dst_size;
  assert(dst_size <= sizeof device->image);
  memcpy(device->image, dst, dst_size);
  free(dst);

  // Pixels are now raw
  device->compression = PLAYER_CAMERA_COMPRESS_RAW;

#else

  PLAYERC_ERR("JPEG decompression support was not included at compile-time");
 
#endif

  return;
}



