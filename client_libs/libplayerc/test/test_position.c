/***************************************************************************
 * Desc: Tests for the position device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for position device.
int test_position(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_position_t *device;

  printf("device [position] index [%d]\n", index);

  device = playerc_position_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_position_subscribe(device, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("position: [%6.3f] [%6.3f] [%6.3f] [%d]\n",
             device->px, device->py, device->pa, device->stall);
    }
    else
      FAIL();
  }
  
  TEST("unsubscribing");
  if (playerc_position_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_position_destroy(device);
  
  return 0;
}

