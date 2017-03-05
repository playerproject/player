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
/*
 * $Id$
 *
 * a test for the C++ Blobfinder Proxy
 */

#include "test.h"
#include <unistd.h>

int
test_blobfinder(PlayerClient* client, int index)
{
  unsigned char access;
  BlobfinderProxy sp(client,index,'c');

  printf("device [blobfinder] index [%d]\n", index);

  TEST("subscribing (read)");
  if((sp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", sp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", sp.driver_name);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();


  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    sp.Print();
  }

  TEST("setting tracking color (auto)");
  if(sp.SetTrackingColor() < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting tracking color (manual)");
  if(sp.SetTrackingColor(40, 80, 120, 160, 200, 240) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting tracking color (manual)");
  if(sp.SetTrackingColor(40, 80, 120, 160, 200, 240) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting contrast");
  if(sp.SetContrast(175) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting brightness");
  if(sp.SetBrightness(175) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting autogain (on)");
  if(sp.SetAutoGain(1) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting autogain (off)");
  if(sp.SetAutoGain(0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting color mode (on)");
  if(sp.SetColorMode(1) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting color mode (off)");
  if(sp.SetColorMode(0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("setting all imager params");
  if(sp.SetImagerParams(50,75,1,1) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();



  TEST("unsubscribing");
  if((sp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

