/*
 * $Id$
 *
 * a test for the C++ LogProxy
 */

#include "test.h"
#include <unistd.h>

int
test_log(PlayerClient* client, int index)
{
  unsigned char access;
  LogProxy lp(client,index,'c');

  printf("device [log] index [%d]\n", index);

  TEST("subscribing (read)");
  if((lp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", lp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", lp.driver_name);

  TEST("getting type/state");
  if(lp.GetState() < 0)
  {
    FAIL();
    return(-1);
  }
  printf("type: %u  state: %u\n", lp.type, lp.state);
  PASS();

  if(lp.type == PLAYER_LOG_TYPE_WRITE)
  {
    TEST("enable logging");
    if(lp.SetWriteState(1) < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();
    TEST("disable logging");
    if(lp.SetWriteState(0) < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();
    TEST("change log filename");
    if(lp.SetFilename("foo") < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();
  }
  else
  {
    TEST("enable playback");
    if(lp.SetReadState(1) < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();
    TEST("disable playback");
    if(lp.SetReadState(0) < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();
    TEST("rewind playback");
    if(lp.Rewind() < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();
  }

  TEST("unsubscribing");
  if((lp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

