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

  TEST("subscribing (read)");
  if((bp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("send message");
  if(bp.Write(msg, strlen(msg)+1) >= 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  sleep(1);

  TEST("receive message");
  if(bp.Read(rep,sizeof(rep)) >= 0)
    PASS();
  else
  {
    FAIL();
    return(-1);
  }

  TEST("compare messages");
  if(!strncmp(msg,rep,strlen(msg)))
    PASS();
  else
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

