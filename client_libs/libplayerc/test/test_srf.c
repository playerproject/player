/***************************************************************************
 * Desc: Tests for the laser device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic srf test
int test_srf(playerc_client_t *client, int index)
{
  int t, i;
  void *rdevice;
  playerc_srf_t *device;

  double min, max;
  int resolution, intensity;

  printf("device [srf] index [%d]\n", index);

  device = playerc_srf_create(client, index);

  TEST("subscribing (read)");
  if (playerc_srf_subscribe(device, PLAYER_READ_MODE) == 0)
    PASS();
  else
    FAIL();
  
  TEST("set configuration");
  min = -M_PI/2;
  max = +M_PI/2;
  resolution = 100;
  intensity = 1;
  if (playerc_srf_set_config(device, min, max, resolution, intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("get configuration");
  if (playerc_srf_get_config(device, &min, &max, &resolution, &intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("check configuration sanity");
  if (abs(min + M_PI/2) > 0.01 || abs(max - M_PI/2) > 0.01)
    FAIL();
  else if (resolution != 100 || intensity != 1)
    FAIL();
  else
    PASS();
  
  TEST("getting geometry");
  if (playerc_srf_get_geom(device) == 0)
    PASS();
  else
    FAIL();
  printf("srf geom: [%6.3f %6.3f %6.3f] [%6.3f %6.3f]\n",
         device->pose[0], device->pose[1], device->pose[2], device->size[0], device->size[1]);

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();
      printf("srf: [%d] ", device->scan_count);
      for (i = 0; i < 3; i++)
        printf("[%6.3f, %6.3f] ", device->scan[i][0], device->scan[i][1]);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_srf_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_srf_destroy(device);
  
  return 0;
}


