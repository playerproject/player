/***************************************************************************
 * Desc: Tests for the GPS device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <math.h>
#include "test.h"
#include "playerc.h"


// Basic test for GPS device.
int test_gps(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_gps_t *device;

  printf("device [gps] index [%d]\n", index);

  device = playerc_gps_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_gps_subscribe(device, PLAYER_ALL_MODE) != 0)
  {
    FAIL();
    return -1;
  }
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
      printf("gps: [%+14.3f] utc [%+14.3f] lon/lat [%+11.7f %+11.7f] alt [%+7.3f] "
             "utm [%.3f %.3f] hdop [%.3f] sats [%d %2d]\n",
             device->info.datatime, device->utc_time,
             device->lon, device->lat, device->alt,
             device->utm_e, device->utm_n,
             device->hdop, 
             device->quality, device->sat_count);
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

