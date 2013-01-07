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
 * Desc: Tests for the GPS device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <math.h>
#include "test.h"
#include "playerc.h"


// Basic test for GPS device.
int test_gps(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_gps_t *device;

  printf("device [gps] index [%d]\n", index);

  device = playerc_gps_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_gps_subscribe(device, PLAYER_ALL_MODE) != 0)
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
      printf("gps: [%+14.3f] utc [%+14.3f] lon/lat [%+11.7f %+11.7f] alt [%+7.3f] "
             "utm [%.3f %.3f] hdop [%.3f] sats [%d %2d]\n",
             device->info.datatime, device->utc_time,
             device->lon, device->lat, device->alt,
             device->utm_e, device->utm_n,
             device->hdop, 
             device->quality, device->sat_count);
    }
    else
      FAIL();
  }
  
  TEST("unsubscribing");
  if (playerc_gps_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_gps_destroy(device);
  
  return 0;
}

