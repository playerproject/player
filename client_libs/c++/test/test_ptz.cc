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
  PtzProxy zp(client,index,'c');

  printf("device [ptz] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((zp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", zp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", zp.driver_name);

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
  if(zp.SetCam(DTOR(90),0,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("panning right");
  if(zp.SetCam(DTOR(-90),0,0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("tilting up");
  if(zp.SetCam(0,DTOR(25),0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("tilting down");
  if(zp.SetCam(0,DTOR(-25),0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("zooming in");
  if(zp.SetCam(0,0,DTOR(10)) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("zooming out");
  if(zp.SetCam(0,0,DTOR(60)) < 0)
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

