/***************************************************************************
 * Desc: Tests for the sonar device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic frf test
int test_frf(playerc_client_t *client, int index)
{
  int t, i;
  void *rdevice;
  playerc_frf_t *device;

  printf("device [frf] index [%d]\n", index);

  device = playerc_frf_create(client, index);

  TEST("subscribing (read)");
  if (playerc_frf_subscribe(device, PLAYER_READ_MODE) == 0)
    PASS();
  else
    FAIL();

  TEST("getting geometry");
  if (playerc_frf_get_geom(device) == 0)
    PASS();
  else
    FAIL();

  printf("frf geom: ");
  for (i = 0; i < device->pose_count; i++)
    printf("[%6.3f %6.3f %6.3f] ", device->poses[i][0], device->poses[i][1], device->poses[i][2]);
  printf("\n");

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();
      printf("frf range: [%d] ", device->scan_count);
      for (i = 0; i < 3; i++)
        printf("[%6.3f] ", device->scan[i]);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_frf_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_frf_destroy(device);
  
  return 0;
}


