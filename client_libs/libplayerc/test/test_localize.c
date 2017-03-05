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
 * Desc: Tests for the localize device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic localize test
int test_localize(playerc_client_t *client, int index)
{
  int t, i;
  void *rdevice;
  playerc_localize_t *device;

  //double min, max;
  //int resolution, intensity;

  printf("device [localize] index [%d]\n", index);

  device = playerc_localize_create(client, index);

  TEST("subscribing (read)");
  if (playerc_localize_subscribe(device, PLAYER_READ_MODE) == 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  // deprecated: get map from map interface now
#if 0
  TEST("get map");
  if (playerc_localize_get_map(device) == 0)
    PASS();
  else
    FAIL();
#endif

  /* FIX
  TEST("set configuration");
  min = -M_PI/2;
  max = +M_PI/2;
  resolution = 100;
  intensity = 1;
  if (playerc_localize_set_config(device, min, max, resolution, intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("get configuration");
  if (playerc_localize_get_config(device, &min, &max, &resolution, &intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("check configuration sanity");
  if (abs(min + M_PI/2) > 0.01 || abs(max - M_PI/2) > 0.01)
    FAIL();
  else if (resolution != 100 || intensity != 1)
    FAIL();
  else
    PASS();
  */

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();

      printf("localize: [%d %14.3f] [%d] ",
             device->pending_count, device->pending_time, device->hypoth_count);
      for (i = 0; i < device->hypoth_count; i++)
        printf("[%6.3f, %6.3f %6.3f] ",
               device->hypoths[i].mean[0], device->hypoths[i].mean[1], device->hypoths[i].mean[2]);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_localize_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_localize_destroy(device);
  
  return 0;
}


