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
/***************************************************************************
 * Desc: Tests for the speech device
 * Author: Alexis MAldonado
 * Date: 4 May 2007
 **************************************************************************/

#include <unistd.h>

#include "test.h"
#include "playerc.h"


// Just write something to a speech device.
int test_speech(playerc_client_t *client, int index)
{
  //int t;
  //void *rdevice;
  playerc_speech_t *device;
  char text[]="Hello World!";
  char text2[]="12345678901234567890123456789012345678901234567890";

  printf("device [speech] index [%d]\n", index);

  device = playerc_speech_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_speech_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();


        TEST1("writing data (attempt %d)",1);

        if (playerc_speech_say(device, text  )  != 0) {
            FAIL();
        } else {
          PASS();
        }
        usleep(1000000);  //some time to see the effect

        TEST1("writing data (attempt %d)",2);

        TEST1("Printing: %s",text2);
        if (playerc_speech_say(device, text2  )  != 0) {
            FAIL();
        } else {
          PASS();
        }
        usleep(1000000);  //some time to see the effect



  TEST("unsubscribing");
  if (playerc_speech_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_speech_destroy(device);
  
  return 0;
}

