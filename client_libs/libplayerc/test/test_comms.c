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
  int i;
  char msg[128];
  void *rdevice;
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

  for (i = 0; i < 10; i++)
  {
    TEST1("sending comms message [%d]", i);
    snprintf(msg, sizeof(msg), "this is message (%d)", i);
    if (playerc_comms_send(comms, msg, strlen(msg) + 1) != 0)
      FAIL();
    else
      PASS();
  }

  for (i = 0; i < 10; i++)
  {
    TEST1("reading data (attempt %d)", i);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == comms)
    {
      PASS();
      if (comms->msg)
        printf("recv : %d [%s]\n", comms->msg_len, comms->msg);
    }
    else
      FAIL();
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




