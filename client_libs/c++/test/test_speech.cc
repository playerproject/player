/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_speech(PlayerClient* client, int index)
{
  unsigned char access;
  SpeechProxy sp(client,index,'c');

  printf("device [speech] index [%d]\n", index);

  TEST("subscribing (write)");
  if((sp.ChangeAccess(PLAYER_WRITE_MODE,&access) < 0) ||
     (access != PLAYER_WRITE_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", sp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", sp.driver_name);

  for(int t = 0; t < 3; t++)
  {
    TEST1("sending \"hello\" (attempt %d)", t);

    if(sp.Say("hello") < 0)
    {
      FAIL();
      return(-1);
    }
    sleep(2);
    PASS();
  }

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

