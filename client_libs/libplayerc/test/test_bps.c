/***************************************************************************
 * Desc: Tests for the BPS device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for BPS device.
int test_bps(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_bps_t *device;

  printf("device [bps] index [%d]\n", index);

  device = playerc_bps_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_bps_subscribe(device, PLAYER_ALL_MODE) < 0)
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
      printf("bps: [%6.3f] [%6.3f] [%6.3f]\n",
             device->px, device->py, device->pa);
    }
    else
      FAIL();
  }
  
  TEST("unsubscribing");
  if (playerc_bps_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_bps_destroy(device);
  
  return 0;
}

