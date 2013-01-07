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
 * Desc: Tests for the position2d device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <unistd.h>

#include "test.h"
#include "playerc.h"


// Basic test for position2d device.
int test_position2d(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_position2d_t *device;

  printf("device [position2d] index [%d]\n", index);

  device = playerc_position2d_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_position2d_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting geometry");
  if (playerc_position2d_get_geom(device) == 0)
    PASS();
  else
    FAIL();
  printf("position2d geom: [%6.3f %6.3f %6.3f] [%6.3f %6.3f]\n",
         device->pose[0], device->pose[1], device->pose[2], device->size[0], device->size[1]);
  
  for (t = 0; t < 30; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("position2d: [%14.3f] [%6.3f] [%6.3f] [%6.3f] [%d]\n",
             device->info.datatime, device->px, device->py, device->pa, device->stall);
    }
    else
    {
      //printf("error: %s", playerc_error_str());
      FAIL();
      break;
    }
  }

  TEST("moving forward");
  if(playerc_position2d_set_cmd_vel(device, 0.1, 0.0, 0.0, 1) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }

  TEST("moving backward");
  if(playerc_position2d_set_cmd_vel(device, -0.1, 0.0, 0.0, 1) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }

  TEST("turning right");
  if(playerc_position2d_set_cmd_vel(device, 0.0, 0.0, DTOR(-25.0), 1) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }

  TEST("turning left");
  if(playerc_position2d_set_cmd_vel(device, 0.0, 0.0, DTOR(25.0), 1) < 0)
    FAIL();
  else
  {
    sleep(3);
    PASS();
  }

  TEST("stopping");
  if(playerc_position2d_set_cmd_vel(device, 0.0, 0.0, 0.0, 1) < 0)
    FAIL();
  else
    PASS();
  
  TEST("unsubscribing");
  if (playerc_position2d_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_position2d_destroy(device);
  
  return 0;
}

