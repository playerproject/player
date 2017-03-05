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
 * Desc: Tests for the map device
 * Author: Brian Gerkey
 * Date: June 2004
 # CVS: $Id$
 **************************************************************************/

#include <math.h>
#include "test.h"
#include "playerc.h"


// Basic test for map device.
int test_map(playerc_client_t *client, int index)
{
//   int t;
//   void *rdevice;
  playerc_map_t *device;

  printf("device [map] index [%d]\n", index);

  device = playerc_map_create(client, index);

  TEST("subscribing (read)");
  if (playerc_map_subscribe(device, PLAYER_OPEN_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("reading map");
  if(playerc_map_get_map(device) != 0)
  {
    FAIL();
    return -1;
  }
  printf("read a %d X %d map @ %.3f m/cell\n",
         device->width, device->height, device->resolution);
  PASS();

  TEST("unsubscribing");
  if (playerc_map_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_map_destroy(device);
  
  return 0;
}

