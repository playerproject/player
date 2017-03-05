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
 * Desc: Tests for the PTZ device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <math.h>
#include "test.h"
#include "playerc.h"


// Basic ptz test
int test_ptz(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_ptz_t *device;
  double period;

  printf("device [ptz] index [%d]\n", index);

  device = playerc_ptz_create(client, index);

  TEST("subscribing (read)");
  if (0 == playerc_ptz_subscribe(device, PLAYER_OPEN_MODE))
    PASS();
  else
    FAIL();

  period = 10 / M_PI * 2;

  for (t = 0; t < 20; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("ptz: [%d %d %d]\n",
             (int) (device->pan * 180 / M_PI),
             (int) (device->tilt * 180 / M_PI),
             (int) (device->zoom * 180 / M_PI));
    }
    else
    {
      FAIL();
      break;
    }

    TEST1("writing data (attempt %d)", t);
    if (playerc_ptz_set(device,
                        sin(t / period) * M_PI / 2,
                        sin(t / period) * M_PI / 3,
                        (1 - t / 20.0) * M_PI) != 0)
    {
      FAIL();
      break;
    }
    PASS();
  }

  TEST1("writing data (attempt %d)", t);
  if (playerc_ptz_set(device, 0, 0, M_PI) != 0)
    FAIL();
  else
    PASS();

  TEST("unsubscribing");
  if (playerc_ptz_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();

  playerc_ptz_destroy(device);

  return 0;
}


