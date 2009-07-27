/***************************************************************************
 * Desc: Tests for the joystick device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for joystick device.
int test_joystick(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_joystick_t *device;

  printf("device [joystick] index [%d]\n", index);

  device = playerc_joystick_create(client, index);

  TEST("subscribing (read)");
  if (playerc_joystick_subscribe(device, PLAYER_READ_MODE) != 0)
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
      printf("joystick: [%.3f] [%.3f] [%X]\n",
             device->px, device->py, device->buttons);
    }
    else
      FAIL();
  }
  
  TEST("unsubscribing");
  if (playerc_joystick_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_joystick_destroy(device);
  
  return 0;
}

