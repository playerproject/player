/*
 * $Id$
 *
 * a test for the C++ WifiProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_wifi(PlayerClient* client, int index)
{
  unsigned char access;
  WiFiProxy wp(client,index,'c');

  printf("device [wifi] index [%d]\n", index);

  TEST("subscribing (read)");
  if((wp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", wp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", wp.driver_name);

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    wp.Print();
  }


  TEST("unsubscribing");
  if((wp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

