/***************************************************************************
 * Desc: Tests for the motor device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for motor device.
int test_motor(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_motor_t *device;

  printf("device [motor] index [%d]\n", index);

  device = playerc_motor_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_motor_subscribe(device, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 30; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("motor: [%14.3f] [%6.3f] [%6.3f] [%d]\n",
             device->info.datatime, device->pt, device->vt, device->stall);
    }
    else
    {
      //printf("error: %s", playerc_error_str());
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_motor_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_motor_destroy(device);
  
  return 0;
}

