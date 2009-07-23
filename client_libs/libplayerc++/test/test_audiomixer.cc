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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "test.h"
#include <unistd.h>
#include <math.h>
//#include <linux/soundcard.h>

int
test_audiomixer(PlayerClient* client, int index)
{
  unsigned char access;
  AudioMixerProxy am(client,index,'c');

  printf("device [audiodsp] index [%d]\n", index);

  TEST("subscribing (all)");
  if((am.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", am.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", am.driver_name);

  TEST("get configuration");
  if(am.GetConfigure())
  {
    FAIL();
    return(-1);
  } else {
    am.Print();
    PASS();
  }

  TEST("Set master volume(50,75)");
  if(am.SetMaster(50,75))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("Set PCM volume(75,50)");
  if(am.SetPCM(75,50))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("Set Line volume(100,75)");
  if(am.SetPCM(100,75))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("Set Mic volume(100,100)");
  if(am.SetPCM(100,100))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("Set IGain(85)");
  if(am.SetIGain(85))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("Set OGain(95)");
  if(am.SetIGain(95))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("sanity check");
  if(am.GetConfigure())
  {
    FAIL();
    return(-1);
  } else {
    am.Print();
    PASS();
  }

  TEST("unsubscribing");
  if((am.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
} 

