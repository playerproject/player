/***************************************************************************
 * Desc: Tests for the broadcast device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic broadcast device test
int test_broadcast(playerc_client_t *client, int index)
{
  int i, j, len;
  char msg[128];
  playerc_broadcast_t *broadcast;

  printf("device [broadcast] index [%d]\n", index);

  broadcast = playerc_broadcast_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_broadcast_subscribe(broadcast, PLAYER_ALL_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  // Send some messages
  for (i = 0; i < 2; i++)
  {
    for (j = 0; j < 5; j++)
    {
      TEST1("sending broadcast message [%d]", i);
      snprintf(msg, sizeof(msg), "this is message [%d:%d]", i, j);
      if (playerc_broadcast_send(broadcast, msg, strlen(msg) + 1) != 0)
        FAIL();
      else
        PASS();
    }

    for (j = 0; j < 10; j++)
    {
      TEST("receiving broadcast message");
      len = playerc_broadcast_recv(broadcast, msg, sizeof(msg));
      if (len == 0)
      {
        PASS();
        break;
      }
      else if (len < 0)
        FAIL();
      else
        PASS();
      printf("recv : %s\n", msg);
    }
  }
  
  TEST("unsubscribing");
  if (playerc_broadcast_unsubscribe(broadcast) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_broadcast_destroy(broadcast);
  
  return 0;
}




