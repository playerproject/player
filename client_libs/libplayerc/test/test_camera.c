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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */
/***************************************************************************
 * Desc: Tests for the camera device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

// Basic test for camera device.
int test_camera(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_camera_t *device;
  char filename[128];
  int csize, usize;

  printf("device [camera] index [%d]\n", index);

  device = playerc_camera_create(client, index);

  TEST("subscribing (read)");
  if (playerc_camera_subscribe(device, PLAYER_OPEN_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();

      // Decompress the image
      csize = device->image_count;
      playerc_camera_decompress(device);
      usize = device->image_count;

      printf("camera: [w %d h %d d %d] [%d/%d bytes]\n",
             device->width, device->height, device->bpp, csize, usize);

      snprintf(filename, sizeof(filename), "camera_%03d.ppm", t);
      printf("camera: saving [%s]\n", filename);
      playerc_camera_save(device, filename);
    }
    else
      FAIL();
  }

  TEST("unsubscribing");
  if (playerc_camera_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  playerc_camera_destroy(device);

  return 0;
}
