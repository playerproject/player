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
 * Author: Richard Vaughan
 * Date: 1 June 2006
 # CVS: $Id$
 **************************************************************************/

#include <libplayercommon/playercommon.h>

#include "test.h"
#include "playerc.h"


// Basic test for simulation device.
int test_simulation(playerc_client_t *client, int index)
{
  double x,y,a;
  //void *rdevice;
  playerc_simulation_t *device;
  int fr, col;

  printf("device [simulation] index [%d]\n", index);

  device = playerc_simulation_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_simulation_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting pose for model robot1");
  if (playerc_simulation_get_pose2d(device, "robot1", &x, &y, &a) == 0)
  {
    PASS();
    printf("pose: (%.3f, %.3f, %.3f)\n", x,y,RTOD(a));
  }
  else
    FAIL();

  TEST("setting pose for model robot1 to (0,0,0)");
  if (playerc_simulation_set_pose2d(device, "robot1", 0, 0, 0) == 0)
    PASS();
  else
    FAIL();

  puts("Sleeping...");
  sleep(3);

  TEST("returning model robot1 to original pose");
  if (playerc_simulation_set_pose2d(device, "robot1", x, y, a) == 0)
    PASS();
  else
    FAIL();
  
  TEST("setting property \"fiducial_return\" for model robot1 to 42");
  fr = 42;
  if (playerc_simulation_set_property(device, "robot1", "_mp_fiducial_return", &fr, sizeof(fr) ) == 0)
    PASS();
  else
    FAIL();
  
  col =  0xFF00;
  TEST("setting property \"color\" for model robot1 to 0x00FF00 (green)");
  if (playerc_simulation_set_property(device, "robot1", "_mp_color", &col, sizeof(col) ) == 0)    
    PASS();
  else
    FAIL();
  
  sleep(1);
  
  col = 0xFF;
  TEST("setting property \"color\" for model robot1 to 0x0000FF (blue)");
  if (playerc_simulation_set_property(device, "robot1", "_mp_color", &col, sizeof(col)) == 0)
    PASS();
  else
    FAIL();
  
  sleep(1);


  col = 0xFF0000;
  TEST("setting property \"color\" for model robot1 to 0xFF0000 (red)");
  if (playerc_simulation_set_property(device, "robot1", "_mp_color", &col, sizeof(col)) == 0)
    PASS();
  else
    FAIL();
  
  TEST("unsubscribing");
  if (playerc_simulation_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_simulation_destroy(device);
  
  return 0;
}

