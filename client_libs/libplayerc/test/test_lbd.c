/***************************************************************************
 * Desc: Tests for the LBD (laser beacon detector) device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Basic test for the laser beacon device.
int test_lbd(playerc_client_t *client, int index)
{
  int t, i;
  int bit_count; double bit_width;
  void *rdevice;
  playerc_lbd_t *device;

  printf("device [lbd] index [%d]\n", index);

  device = playerc_lbd_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_lbd_subscribe(device, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();
    
  TEST("set configuration");
  if (playerc_lbd_set_config(device, 5, 0.05) == 0)
    PASS();
  else
    FAIL();

  TEST("get configuration");
  if (playerc_lbd_get_config(device, &bit_count, &bit_width) == 0)
    PASS();
  else
    FAIL();

  TEST("check configuration sanity");
  if (bit_count != 5 || bit_width != 0.05)
    FAIL();
  else
    PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();

      printf("lbd: [%d] ", device->beacon_count);
      for (i = 0; i < MIN(3, device->beacon_count); i++)
        printf("[%d %6.3f, %6.3f, %6.3f] ", device->beacons[i].id,
               device->beacons[i].range, device->beacons[i].bearing,
               device->beacons[i].orient);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_lbd_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_lbd_destroy(device);
  
  return 0;
}
