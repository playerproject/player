/***************************************************************************
 * Desc: Tests for the GPS device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for GPS device.
int test_gps(playerc_client_t *client, int robot, int index)
{
  int t;
  void *rdevice;
  playerc_gps_t *device;

  printf("device [gps] index [%d]\n", index);

  device = playerc_gps_create(client, robot, index);

  TEST("subscribing (read/write)");
  if (playerc_gps_subscribe(device, PLAYER_ALL_MODE) != 0)
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
      printf("gps: [%6.3f] [%6.3f] [%6.3f]\n",
             device->px, device->py, device->pa);
    }
    else
      FAIL();
  }
  
  TEST("unsubscribing");
  if (playerc_gps_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_gps_destroy(device);
  
  return 0;
}

