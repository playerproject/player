/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_vision(PlayerClient* client, int index)
{
  unsigned char access;
  BlobfinderProxy vp(client,index,'c');

  printf("device [vision] index [%d]\n", index);

  TEST("subscribing (read)");
  if((vp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", vp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", vp.driver_name);

  if(!use_stage)
  {
    // let ACTS start up
    TEST("waiting for ACTS to start up");
    for(int i=0;i<100;i++)
      client->Read();
    puts("done.");
  }

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    vp.Print();
  }

  TEST("unsubscribing");
  if((vp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

