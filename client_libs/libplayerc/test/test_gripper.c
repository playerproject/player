/***************************************************************************
 * Desc: Tests for the position2d device
 * Author: Richard Vaughan, based on Andrew Howard's test_laser.c
 * Date: 9 October 2005
 # CVS: $Id$
 **************************************************************************/

#include <unistd.h>

#include "test.h"
#include "playerc.h"

int test_gripper(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_gripper_t *device;
  
  printf("device [gripper] index [%d]\n", index);

  device = playerc_gripper_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_gripper_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 5; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      playerc_gripper_print( device, "gripper" );
    }
    else
    {
      //printf("error: %s", playerc_error_str());
      FAIL();
      break;
    }
  }

  TEST("closing gripper");
  if(playerc_gripper_set_cmd(device, GRIPclose, 0 ) < 0)
    FAIL();
  else
  {
    sleep(3);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    playerc_gripper_print( device, "gripper" );

    PASS();
  }


  TEST("opening gripper");
  if(playerc_gripper_set_cmd(device, GRIPopen, 0 ) < 0)
    FAIL();
  else
  {
    sleep(3);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    playerc_gripper_print( device, "gripper" );

    PASS();
  }

  
  TEST("unsubscribing");
  if (playerc_gripper_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_gripper_destroy(device);
  
  return 0;
}

