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
 * Desc: Tests for the truth device (must run Stage)
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#include <math.h>


// Basic test for truth device.
int test_truth(playerc_client_t *client, int index)
{
  int t;

  double pos_i[3], rot_i[3];
  double pos_f[3], rot_f[3];
  
  void *rdevice;
  playerc_truth_t *device;

  printf("device [truth] index [%d]\n", index);

  device = playerc_truth_create(client, index);

  TEST("subscribing (read)");
  if (playerc_truth_subscribe(device, PLAYER_READ_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 3; t++)
  {
    TEST("getting pose (req/rep)");
    if (playerc_truth_get_pose(device,
                               pos_f + 0, pos_f + 1, pos_f + 2,
                               rot_f + 1, rot_f + 1, rot_f + 2) != 0)
    {
      FAIL();
      return -1;
    }
    PASS();
    printf("truth: [%6.3f %6.3f %6.3f]\n", pos_f[0], pos_f[1], rot_f[2]);
  }
  
  TEST("setting pose");
  pos_i[0] = 2; pos_i[1] = 3; pos_i[2] = 0;
  rot_i[0] = 0; rot_i[1] = 0; rot_i[2] = M_PI/2;
  if (playerc_truth_set_pose(device,
                             pos_i[0], pos_i[1], pos_i[2],
                             rot_i[0], rot_i[1], rot_i[2]) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting pose (req/rep)");
  if (playerc_truth_get_pose(device,
                             pos_f + 0, pos_f + 1, pos_f + 2,
                             rot_f + 1, rot_f + 1, rot_f + 2) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  printf("truth: [%6.3f %6.3f %6.3f]\n", pos_f[0], pos_f[1], rot_f[2]);

  TEST("checking values for consitency");
  if (fabs(pos_f[0] - pos_i[0]) > 0.001 ||
      fabs(pos_f[1] - pos_i[1]) > 0.001 ||
      fabs(rot_f[2] - rot_i[2]) > 0.001)
    FAIL();
  else
    PASS();

  for (t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("truth: [%6.3f %6.3f %6.3f]\n",
             device->pos[0], device->pos[1], device->rot[2]);
    }
    else
      FAIL();
  }

  TEST("unsubscribing");
  if (playerc_truth_unsubscribe(device) != 0)
    FAIL();
  else
    PASS();
  
  playerc_truth_destroy(device);
  
  return 0;
}

