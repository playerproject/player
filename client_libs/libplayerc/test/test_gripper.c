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
 * Desc: Tests for the gripper device
 * Author: Richard Vaughan, based on Andrew Howard's test_laser.c
 * Date: 9 October 2005
 # CVS: $Id$
 **************************************************************************/

#include <unistd.h>

#include "test.h"
#include "playerc.h"

int test_gripper(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_gripper_t *device;

  printf("device [gripper] index [%d]\n", index);

  device = playerc_gripper_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_gripper_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 5; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      playerc_gripper_printout( device, "gripper" );
    }
    else
    {
      //printf("error: %s", playerc_error_str());
      FAIL();
      break;
    }
  }

  TEST("closing gripper");
  if(playerc_gripper_close_cmd(device) < 0)
    FAIL();
  else
  {
    sleep(3);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    playerc_gripper_printout( device, "gripper" );

    PASS();
  }


  TEST("opening gripper");
  if(playerc_gripper_open_cmd(device) < 0)
    FAIL();
  else
  {
    sleep(3);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    playerc_gripper_printout( device, "gripper" );

    PASS();
  }


  TEST("unsubscribing");
  if (playerc_gripper_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  playerc_gripper_destroy(device);

  return 0;
}

