/*
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002-2003
 * *
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
 * Desc: Stereo device proxy
 * Author: Radu Bogdan Rusu
 * Date: 5 June 2008
 **************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "playerc.h"
#include "error.h"

#if defined (WIN32)
  #define snprintf _snprintf
#endif

// Process incoming data
void playerc_stereo_putmsg (playerc_stereo_t *device,
                            player_msghdr_t *header,
                            void *data);

// Create a new stereo proxy
playerc_stereo_t *playerc_stereo_create (playerc_client_t *client, int index)
{
  playerc_stereo_t *device;

  device = malloc (sizeof (playerc_stereo_t));
  memset (device, 0, sizeof (playerc_stereo_t));

  playerc_device_init (&device->info, client, PLAYER_STEREO_CODE, index,
                      (playerc_putmsg_fn_t) playerc_stereo_putmsg);

  return device;
}

// Destroy a stereo proxy
void playerc_stereo_destroy (playerc_stereo_t *device)
{
  playerc_device_term (&device->info);
  free (device->left_channel.image);
  free (device->right_channel.image);
  free (device->disparity.image);
//  free (device->pointcloud.points);
  free (device->points);
  free (device);

  return;
}

// Subscribe to the stereo device
int playerc_stereo_subscribe (playerc_stereo_t *device, int access)
{
  return playerc_device_subscribe (&device->info, access);
}


// Un-subscribe from the stereo device
int playerc_stereo_unsubscribe (playerc_stereo_t *device)
{
  return playerc_device_unsubscribe (&device->info);
}

// Process incoming data
void playerc_stereo_putmsg (playerc_stereo_t *device,
			    player_msghdr_t *header,
		            void *data)
{
  if((header->type == PLAYER_MSGTYPE_DATA) &&
     (header->subtype == PLAYER_STEREO_DATA_STATE))
  {
    player_stereo_data_t* s_data = (player_stereo_data_t*)data;

    device->left_channel.width        = s_data->left_channel.width;
    device->left_channel.height       = s_data->left_channel.height;
    device->left_channel.bpp          = s_data->left_channel.bpp;
    device->left_channel.format       = s_data->left_channel.format;
    device->left_channel.fdiv         = s_data->left_channel.fdiv;
    device->left_channel.compression  = s_data->left_channel.compression;
    device->left_channel.image_count  = s_data->left_channel.image_count;
    device->left_channel.image        = realloc (device->left_channel.image, sizeof (device->left_channel.image[0]) * device->left_channel.image_count);

    device->right_channel.width        = s_data->right_channel.width;
    device->right_channel.height       = s_data->right_channel.height;
    device->right_channel.bpp          = s_data->right_channel.bpp;
    device->right_channel.format       = s_data->right_channel.format;
    device->right_channel.fdiv         = s_data->right_channel.fdiv;
    device->right_channel.compression  = s_data->right_channel.compression;
    device->right_channel.image_count  = s_data->right_channel.image_count;
    device->right_channel.image        = realloc (device->right_channel.image, sizeof (device->right_channel.image[0]) * device->right_channel.image_count);

    device->disparity.width        = s_data->disparity.width;
    device->disparity.height       = s_data->disparity.height;
    device->disparity.bpp          = s_data->disparity.bpp;
    device->disparity.format       = s_data->disparity.format;
    device->disparity.fdiv         = s_data->disparity.fdiv;
    device->disparity.compression  = s_data->disparity.compression;
    device->disparity.image_count  = s_data->disparity.image_count;
    device->disparity.image        = realloc (device->disparity.image, sizeof (device->disparity.image[0]) * device->disparity.image_count);

    if (device->left_channel.image)
      memcpy (device->left_channel.image, s_data->left_channel.image, device->left_channel.image_count);
    else if (device->left_channel.image_count != 0)
      PLAYERC_ERR1 ("failed to allocate memory for left image, needed %zd bytes\n", sizeof (device->left_channel.image[0]) * device->left_channel.image_count);

    if (device->right_channel.image)
      memcpy (device->right_channel.image, s_data->right_channel.image, device->right_channel.image_count);
    else if (device->right_channel.image_count != 0)
      PLAYERC_ERR1 ("failed to allocate memory for right image, needed %zd bytes\n", sizeof (device->right_channel.image[0]) * device->right_channel.image_count);

    if (device->disparity.image)
      memcpy (device->disparity.image, s_data->disparity.image, device->disparity.image_count);
    else if (device->disparity.image_count != 0)
      PLAYERC_ERR1 ("failed to allocate memory for disparity image, needed %zd bytes\n", sizeof (device->disparity.image[0]) * device->disparity.image_count);

/*    device->pointcloud.points_count = s_data->pointcloud.points_count;
    device->pointcloud.points = realloc (device->pointcloud.points, device->pointcloud.points_count * sizeof (device->pointcloud.points[0]));
    if (device->pointcloud.points)
      memcpy (device->pointcloud.points, s_data->pointcloud.points, sizeof (player_pointcloud3d_element_t)*device->pointcloud.points_count);*/
    device->points_count = s_data->points_count;
    device->points = realloc (device->points, device->points_count * sizeof (device->points[0]));
    if (device->points)
      memcpy (device->points, s_data->points, sizeof (playerc_pointcloud3d_stereo_element_t)*device->points_count);
  }
  else
    PLAYERC_WARN2 ("skipping stereo message with unknown type/subtype: %s/%d\n",
                   msgtype_to_str(header->type), header->subtype);
}
