/*
 * $Id$
 *
 * a test for the C++ LaserProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_broadcast(PlayerClient* client, int index)
{
  unsigned char access;
  CommsProxy bp(client,index);
  char msg[32];
  char rep[32];

  strcpy(msg,"hello");

  printf("device [broadcast] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((bp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", bp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", bp.driver_name);

  TEST("send message");
  if(bp.Write(msg, strlen(msg)+1) >= 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  sleep(1);


  for(int t = 0; t < 3; t++)
  {
    TEST("receive message");
    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }
    PASS();

    if (bp.msg_len > 0)
    {
      bp.Print();
      
      TEST("compare messages");
      if(!strncmp(msg,(const char*) bp.msg,strlen(msg)))
      {
        PASS();
        break;
      }
      else
      {
        FAIL();
        return(-1);
      }
    }
  }

  TEST("got message");
  if (bp.msg_len == 0)
  {
    FAIL();
    return(-1);
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

