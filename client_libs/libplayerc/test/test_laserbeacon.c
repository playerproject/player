/***************************************************************************
 * Desc: Tests for the position device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Basic test for the laser beacon device.
int test_laserbeacon(playerc_client_t *client, int index)
{
  int t, i;
  playerc_laserbeacon_t *laserbeacon;

  printf("device [laserbeacon] index [%d]\n", index);

  laserbeacon = playerc_laserbeacon_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_laserbeacon_subscribe(laserbeacon, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      return -1;
    }
    PASS();

    printf("laserbeacon: [%d] ", laserbeacon->beacon_count);
    for (i = 0; i < MIN(3, laserbeacon->beacon_count); i++)
      printf("[%d %6.3f, %6.3f, %6.3f] ", laserbeacon->beacons[i].id,
             laserbeacon->beacons[i].range, laserbeacon->beacons[i].bearing,
             laserbeacon->beacons[i].orient);
    printf("\n");
  }
  
  TEST("unsubscribing");
  if (playerc_laserbeacon_unsubscribe(laserbeacon) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_laserbeacon_destroy(laserbeacon);
  
  return 0;
}
