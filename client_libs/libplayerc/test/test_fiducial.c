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
 * Desc: Tests for the LBD (laser beacon detector) device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <math.h>
#include "test.h"
#include "playerc.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// Basic test for the laser beacon device.
int test_fiducial(playerc_client_t *client, int index)
{
  int t, i;
  //int bit_count; double bit_width;
  void *rdevice;
  playerc_fiducial_t *device;

  printf("device [fiducial] index [%d]\n", index);

  device = playerc_fiducial_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_fiducial_subscribe(device, PLAYER_ALL_MODE) < 0)
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

      printf("fiducial: [%d] ", device->fiducial_count);
      for (i = 0; i < min(3, device->fiducial_count); i++)
        printf("[%d %6.3f %6.3f %6.3f %6.3f %6.3f] ", device->fiducials[i].id,
               device->fiducials[i].pos[0], device->fiducials[i].pos[1],
               device->fiducials[i].range, device->fiducials[i].bearing * 180 / M_PI,
               device->fiducials[i].orient);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_fiducial_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_fiducial_destroy(device);
  
  return 0;
}
