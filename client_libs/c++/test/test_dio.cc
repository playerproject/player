/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_dio(PlayerClient* client, int index)
{
  unsigned char access;
  DIOProxy dp(client,index);

  printf("device [dio] index [%d]\n", index);

  TEST("subscribing (read)");
  if((dp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", dp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", dp.driver_name);

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    dp.Print();
  }

  TEST("unsubscribing");
  if((dp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

