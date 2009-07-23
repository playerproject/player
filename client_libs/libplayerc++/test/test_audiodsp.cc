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
// pasted in from <linux/soundcard.h>:
#define AFMT_S16_LE              0x00000010      /* Little endian signed 16*/

int
test_audiodsp(PlayerClient* client, int index)
{
  unsigned char access;
  AudioDSPProxy ap(client,index,'c');

  printf("device [audiodsp] index [%d]\n", index);

  TEST("subscribing (all)");
  if((ap.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", ap.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", ap.driver_name);


  TEST("set configuration");
  if(ap.Configure(1, 8000, AFMT_S16_LE))
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }

  TEST("get configuration");
  if(ap.GetConfigure())
  {
    FAIL();
    return(-1);
  } else {
    PASS();
  }


  TEST("play chirp");
  unsigned char mseq[64]={
    0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,0,1,0,0,0,0,1,0,0,1,1,1,0,0,
    0,1,1,0,1,0,0,1,0,0,1,0,1,1,1,0,1,1,0,1,1,1,0,0,1,1,0,1,1,0,
    0,1,0,1};
  if(ap.PlayChirp(1000,20,2,mseq,64))
  {
    FAIL();
  } else {
    PASS();
    sleep(3);
  }

  TEST("play tone");
  if(ap.PlayTone(1000,20,1000))
  {
    FAIL();
  } else {
    PASS();
    sleep(3);
  }

  TEST("replay");
  if(ap.Replay())
  {
    FAIL();
  } else {
    PASS();
    sleep(3);
  }
  
  for( int i=0; i<10;i++ )
  {

    TEST1("Reading Data (attempt %d)",i+1);
    if( client->Read() < 0 )
    {
      FAIL();
      return -1;
    } else {
      PASS();
    }

    printf("Freq(1-5):%d,%d,%d,%d,%d\n",ap.freq[0],ap.freq[1],ap.freq[2],ap.freq[3],ap.freq[4]);

  }

  TEST("unsubscribing");
  if((ap.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
} 

