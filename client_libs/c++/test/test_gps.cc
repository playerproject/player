/*
 * $Id$
 *
 * a test for the C++ GpsProxy
 */

#include "playerclient.h"
#include "test.h"

int
test_gps(PlayerClient* client, int index)
{
  unsigned char access;
  GpsProxy gp(client,index);

  printf("device [gps] index [%d]\n", index);

  TEST("subscribing (read)");
  if((gp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    return -1;
  }
  PASS();

  /*
  TEST("warping position");
  if(gp.Warp(100,100,100) < 0)
  {
    FAIL();
    return(-1);
  }
  PASS();
  */

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    gp.Print();
  }

  TEST("unsubscribing (read)");
  if((gp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

