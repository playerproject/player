/***************************************************************************
 * Desc: Tests for the LBD (laser beacon detector) device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

// Basic test for the laser beacon device.
int test_fiducial(playerc_client_t *client, int index)
{
  int t, i;
  //int bit_count; double bit_width;
  void *rdevice;
  playerc_fiducial_t *device;

  printf("device [fiducial] index [%d]\n", index);

  device = playerc_fiducial_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_fiducial_subscribe(device, PLAYER_ALL_MODE) < 0)
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

      printf("fiducial: [%d] ", device->fiducial_count);
      for (i = 0; i < min(3, device->fiducial_count); i++)
        printf("[%d %6.3f, %6.3f, %6.3f] ", device->fiducials[i].id,
               device->fiducials[i].range, device->fiducials[i].bearing,
               device->fiducials[i].orient);
      printf("\n");
    }
    else
    {
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_fiducial_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_fiducial_destroy(device);
  
  return 0;
}
