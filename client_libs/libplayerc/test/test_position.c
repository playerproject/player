/***************************************************************************
 * Desc: Tests for the position device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for position device.
int test_position(playerc_client_t *client, int robot, int index)
{
  int t;
  void *rdevice;
  playerc_position_t *device;

  printf("device [position] index [%d]\n", index);

  device = playerc_position_create(client, robot, index);

  TEST("subscribing (read/write)");
  if (playerc_position_subscribe(device, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting geometry");
  if (playerc_position_get_geom(device) == 0)
    PASS();
  else
    FAIL();
  printf("position geom: [%6.3f %6.3f %6.3f] [%6.3f %6.3f]\n",
         device->pose[0], device->pose[1], device->pose[2], device->size[0], device->size[1]);

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

