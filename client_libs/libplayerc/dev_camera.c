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
#if HAVE_CONFIG_H
  #include "config.h"
#endif

#if HAVE_JPEGLIB_H
  #include "libplayerjpeg/playerjpeg.h"
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "playerc.h"
#include "error.h"

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

    assert(device->image_count <= sizeof(device->image));
    memcpy(device->image, data->image, device->image_count);
  }
  else
    PLAYERC_WARN2("skipping camera message with unknown type/subtype: %d/%d\n",
                 header->type, header->subtype);
  return;
}


// Decompress image data
void playerc_camera_decompress(playerc_camera_t *device)
{
  if (device->compression == PLAYER_CAMERA_COMPRESS_RAW)
    return;

#if HAVE_JPEGLIB_H
  int dst_size;
  unsigned char *dst;

  // Create a temp buffer
  dst_size = device->width * device->height * device->bpp / 8;
  dst = malloc(dst_size);

  // Decompress into temp buffer
  jpeg_decompress(dst, dst_size, device->image, device->image_count);

  // Copy uncompress image
  device->image_count = dst_size;
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
      printf("unsupported image format");
      break;
    }
  }

  fclose(file);

  return;
}

// Set the auto-illumination option on/off
int
playerc_camera_autoillumination(playerc_camera_t *device, int value)
{
  player_camera_autoillumination_config_t config;

  config.value = value;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_CAMERA_REQ_SET_AUTOILLUMINATION,
                                &config, NULL, 0));
}

// Set the modulation frequency
int
playerc_camera_set_modulationfreq(playerc_camera_t *device, int value)
{
  player_camera_modulation_freq_config_t config;

  config.value = value;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_CAMERA_REQ_SET_MODULATION_FREQ,
                                &config, NULL, 0));
}

// Get the modulation frequency
int 
playerc_camera_get_modulationfreq(playerc_camera_t *device, int *value)
{
  player_camera_modulation_freq_config_t config;
  memset(&config, 0, sizeof(config));
  if (playerc_client_request(device->info.client, &device->info, 
                             PLAYER_CAMERA_REQ_GET_MODULATION_FREQ,
			     &config, &config, sizeof(config)) < 0)
    return (-1);
  *value = config.value;
  return 0;
}

// Set the integration time
int
playerc_camera_set_integrationtime(playerc_camera_t *device, int value)
{
  player_camera_integration_time_config_t config;

  config.value = value;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_CAMERA_REQ_SET_INTEGRATION_TIME,
                                &config, NULL, 0));
}

// Get the integration time
int 
playerc_camera_get_integrationtime(playerc_camera_t *device, int *value)
{
  player_camera_integration_time_config_t config;
  memset(&config, 0, sizeof(config));
  if (playerc_client_request(device->info.client, &device->info, 
                             PLAYER_CAMERA_REQ_GET_INTEGRATION_TIME,
			     &config, &config, sizeof(config)) < 0)
    return (-1);
  *value = config.value;
  return 0;
}

// Set threshold (eg. saturation, amplitude, etc)
int
playerc_camera_threshold(playerc_camera_t *device, int subtype, int value)
{
  player_camera_threshold_config_t config;

  config.subtype = subtype;
  config.value   = value;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_CAMERA_REQ_SET_THRESHOLD,
                                &config, NULL, 0));
}

// Set IIR
int
playerc_camera_iir(playerc_camera_t *device, float static_delay, float dynamic_delay)
{
  player_camera_iir_config_t config;

  config.static_delay  = static_delay;
  config.dynamic_delay = dynamic_delay;

  return(playerc_client_request(device->info.client,
                                &device->info,
                                PLAYER_CAMERA_REQ_SET_TEMPORAL_IIR,
                                &config, NULL, 0));
}

