/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_sonar(PlayerClient* client, int index)
{
  unsigned char access;
  SonarProxy sp(client,index);

  printf("device [sonar] index [%d]\n", index);

  TEST("subscribing (read)");
  if((sp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    return -1;
  }
  PASS();

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

  TEST("disabling sonars");
  if(sp.SetSonarState(0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("enabling sonars");
  if(sp.SetSonarState(1) < 0)
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

