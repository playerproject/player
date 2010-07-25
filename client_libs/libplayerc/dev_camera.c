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
 * Desc: Camera proxy.
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 **************************************************************************/
#include <config.h>

#if HAVE_JPEG
  #include "libplayerjpeg/playerjpeg.h"
#endif

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined (WIN32)
  #include <netinet/in.h>
#endif

#include "playerc.h"
#include "error.h"

#if defined (WIN32)
  #define snprintf _snprintf
  #define strdup _strdup
#endif

// Local declarations
void playerc_camera_putmsg(playerc_camera_t *device,
                           player_msghdr_t *header,
                           player_camera_data_t *data,
                           size_t len);

// Create a new camera proxy
playerc_camera_t *playerc_camera_create(playerc_client_t *client, int index)
{
  playerc_camera_t *device;

  device = malloc(sizeof(playerc_camera_t));
  memset(device, 0, sizeof(playerc_camera_t));
  playerc_device_init(&device->info, client, PLAYER_CAMERA_CODE, index,
                      (playerc_putmsg_fn_t) playerc_camera_putmsg);
  return device;
}


// Destroy a camera proxy
void playerc_camera_destroy(playerc_camera_t *device)
{
  playerc_device_term(&device->info);
  free(device->image);
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
void playerc_camera_putmsg(playerc_camera_t *device, player_msghdr_t *header,
                            player_camera_data_t *data, size_t len)
{
  if((header->type == PLAYER_MSGTYPE_DATA) &&
     (header->subtype == PLAYER_CAMERA_DATA_STATE))
  {
    device->width        = data->width;
    device->height       = data->height;
    device->bpp          = data->bpp;
    device->format       = data->format;
    device->fdiv         = data->fdiv;
    device->compression  = data->compression;
    device->image_count  = data->image_count;
    if (device->image_count > 0)
    {
      device->image        = realloc(device->image, sizeof(device->image[0])*device->image_count);
      if (device->image)
        memcpy(device->image, data->image, device->image_count);
      else
        PLAYERC_ERR1("failed to allocate memory for image, needed %u bytes\n", sizeof(device->image[0])*device->image_count);
    } else
    {
      if (device->image) free(device->image);
      device->image = NULL;
    }
  }
  else
    PLAYERC_WARN2("skipping camera message with unknown type/subtype: %s/%d\n",
                 msgtype_to_str(header->type), header->subtype);
  return;
}


// Decompress image data
void playerc_camera_decompress(playerc_camera_t *device)
{
  if (device->compression == PLAYER_CAMERA_COMPRESS_RAW)
  {
    return;
  } else
  {
#if HAVE_JPEG
    // Create a temp buffer
    int dst_size = device->width * device->height * device->bpp / 8;
    unsigned char * dst = malloc(dst_size);

    // Decompress into temp buffer
    jpeg_decompress(dst, dst_size, device->image, device->image_count);

    // Copy uncompress image
    device->image_count = dst_size;
    device->image = realloc(device->image, (sizeof device->image[0]) * device->image_count);
    if (device->image)
      memcpy(device->image, dst, dst_size);
    else
      PLAYERC_ERR1("failed to allocate memory for image, needed %u bytes\n", (sizeof device->image[0]) * device->image_count);
    free(dst);

    // Pixels are now raw
    device->compression = PLAYER_CAMERA_COMPRESS_RAW;

#else

    PLAYERC_ERR("JPEG decompression support was not included at compile-time");

#endif
  }

  return;
}

// Save a camera image
// Assumes the image is RGB888
void playerc_camera_save(playerc_camera_t *device, const char *filename)
{
  int i;
  uint8_t pix;
  FILE *file;

  file = fopen(filename, "w+");
  if (file == NULL)
    return;

  // we need to decompress the image
  playerc_camera_decompress(device);

  // Write ppm header
  fprintf(file, "P6\n%d %d\n%d\n", device->width, device->height, 255);

  // Write data here
  for (i = 0; i < device->image_count; i++)
  {
    if (device->format == PLAYER_CAMERA_FORMAT_RGB888)
    {
      pix = device->image[i];
      fputc(pix, file);
    }
    else if (device->format == PLAYER_CAMERA_FORMAT_MONO8)
    {
      pix = device->image[i];
      fputc(pix, file);
      fputc(pix, file);
      fputc(pix, file);
    }
    else
    {
      fprintf(stderr,"unsupported image format");
      break;
    }
  }

  fclose(file);

  return;
}

// Set source channel.
int
playerc_camera_set_source(playerc_camera_t *device,
                          int source,
                          const char * norm)
{
  player_camera_source_t src;

  memset(&src, 0, sizeof src);
  src.norm_count = strlen(norm) + 1;
  src.norm = strdup(norm);
  if (!(src.norm)) return -1;
  src.source = source;
  if (playerc_client_request(device->info.client, &device->info,
                             PLAYER_CAMERA_REQ_SET_SOURCE,
                             (void*)(&src), NULL) < 0)
  {
    free(src.norm);
    return -1;
  }
  // if the set suceeded copy them locally
  snprintf(device->norm, sizeof device->norm, "%s", src.norm);
  free(src.norm);
  device->source = src.source;
  return 0;
}

// Get the source channel.
int
playerc_camera_get_source(playerc_camera_t *device)
{
  player_camera_source_t * src;

  if (playerc_client_request(device->info.client, &device->info,
                             PLAYER_CAMERA_REQ_GET_SOURCE,
                             NULL, (void**)&src) < 0)
    return -1;
  snprintf(device->norm, sizeof device->norm, "%s", src->norm);
  device->source = src->source;
  player_camera_source_t_free(src);
  return 0;
}

// Force to get current image.
int
playerc_camera_get_image(playerc_camera_t *device)
{
  player_camera_data_t * data;

  if (playerc_client_request(device->info.client, &device->info,
                             PLAYER_CAMERA_REQ_GET_IMAGE,
                             NULL, (void**)&data) < 0)
    return -1;
  device->width        = data->width;
  device->height       = data->height;
  device->bpp          = data->bpp;
  device->format       = data->format;
  device->fdiv         = data->fdiv;
  device->compression  = data->compression;
  device->image_count  = data->image_count;
  if (device->image_count > 0)
  {
    device->image        = realloc(device->image, (sizeof device->image[0]) * device->image_count);
    if (device->image)
      memcpy(device->image, data->image, device->image_count);
    else
      PLAYERC_ERR1("failed to allocate memory for image, needed %u bytes\n", (sizeof device->image[0]) * device->image_count);
  } else
  {
    if (device->image) free(device->image);
    device->image = NULL;
  }
  player_camera_data_t_free(data);
  return 0;
}

// Copy image to some pre-allocated place.
void
playerc_camera_copy_image(playerc_camera_t * device, void * dst, size_t dst_size)
{
  memcpy(dst, device->image, (dst_size <= (device->image_count)) ? dst_size : (device->image_count));
}
// @brief Get given component of given pixel.
unsigned
playerc_camera_get_pixel_component(playerc_camera_t * device, unsigned int x, unsigned int y, int component)
{
  return (unsigned)(device->image[(y * device->width * ((device->bpp)>>3)) + (x * ((device->bpp)>>3)) + component]);
}
