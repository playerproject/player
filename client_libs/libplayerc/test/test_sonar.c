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
 * Desc: Tests for the sonar device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic sonar test
int test_sonar(playerc_client_t *client, int index)
{
  int t, i;
  void *rdevice;
  playerc_sonar_t *device;

  printf("device [sonar] index [%d]\n", index);

  device = playerc_sonar_create(client, index);

  TEST("subscribing (read)");
  if (playerc_sonar_subscribe(device, PLAYER_OPEN_MODE) == 0)
    PASS();
  else
    FAIL();

  TEST("getting geometry");
  if (playerc_sonar_get_geom(device) == 0)
    PASS();
  else
    FAIL();

  printf("sonar geom: ");
  for (i = 0; i < device->pose_count; i++)
    printf("[%6.3f %6.3f %6.3f] ", device->poses[i].px, device->poses[i].py, device->poses[i].pyaw);
  printf("\n");

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();
      printf("sonar range: [%d] ", device->scan_count);
      for (i = 0; i < 3; i++)
        printf("[%6.3f] ", device->scan[i]);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_sonar_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_sonar_destroy(device);
  
  return 0;
}


