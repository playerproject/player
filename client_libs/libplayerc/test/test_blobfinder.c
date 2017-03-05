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
 * Desc: Tests for the vision device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// Basic blobfinder test
int test_blobfinder(playerc_client_t *client, int index)
{
  int t, i;
  void *rdevice;
  playerc_blobfinder_t *device;

  printf("device [blobfinder] index [%d]\n", index);

  device = playerc_blobfinder_create(client, index);

  TEST("subscribing (read)");
  if (playerc_blobfinder_subscribe(device, PLAYER_OPEN_MODE) == 0)
    PASS();
  else
    FAIL();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("blobfinder: [%d, %d] [%d] ", device->width, device->height, device->blobs_count);
      for (i = 0; i < min(3, device->blobs_count); i++)
        printf("[%d : (%d %d) (%d %d %d %d) : %d] ", device->blobs[i].id,
               device->blobs[i].x, device->blobs[i].y,
               device->blobs[i].left, device->blobs[i].top,
               device->blobs[i].right, device->blobs[i].bottom,
               device->blobs[i].area);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }

  TEST("unsubscribing");
  if (playerc_blobfinder_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();

  playerc_blobfinder_destroy(device);

  return 0;
}


