/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_bps(PlayerClient* client, int index)
{
  unsigned char access;
  BpsProxy bp(client,index);

  printf("device [bps] index [%d]\n", index);

  TEST("subscribing (read)");
  if((bp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    return -1;
  }
  PASS();

  // wait for the laser and P2OS to start up
  for(int i=0; i < 30; i++)
    client->Read();

  // add a phony beacon
  TEST("adding a beacon");
  if(bp.AddBeacon(1,100,100,100) < 0)
  {
    FAIL();
    return(-1);
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

    bp.Print();
  }

  TEST("unsubscribing");
  if((bp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

