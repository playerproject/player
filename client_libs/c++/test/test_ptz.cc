/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_ptz(PlayerClient* client, int index)
{
  unsigned char access;
  PtzProxy zp(client,index);

  printf("device [ptz] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((zp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    return -1;
  }
  PASS();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    zp.Print();
  }

  TEST("panning left");
  if(zp.SetCam(90,0,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("panning right");
  if(zp.SetCam(-90,0,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("tilting up");
  if(zp.SetCam(0,25,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("tilting down");
  if(zp.SetCam(0,-25,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("zooming in");
  if(zp.SetCam(0,0,1024) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("zooming out");
  if(zp.SetCam(0,0,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("unsubscribing");
  if((zp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

