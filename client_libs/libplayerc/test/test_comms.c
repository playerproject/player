/***************************************************************************
 * Desc: Tests for the comms device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic comms device test
int test_comms(playerc_client_t *client, int index)
{
  int i, j, len;
  char msg[128];
  playerc_comms_t *comms;

  printf("device [comms] index [%d]\n", index);

  comms = playerc_comms_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_comms_subscribe(comms, PLAYER_ALL_MODE) != 0)
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
      TEST1("sending comms message [%d]", i);
      snprintf(msg, sizeof(msg), "this is message [%d:%d]", i, j);
      if (playerc_comms_send(comms, msg, strlen(msg) + 1) != 0)
        FAIL();
      else
        PASS();
    }

    for (j = 0; j < 10; j++)
    {
      TEST("receiving comms message");
      len = playerc_comms_recv(comms, msg, sizeof(msg));
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
  if (playerc_comms_unsubscribe(comms) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_comms_destroy(comms);
  
  return 0;
}




