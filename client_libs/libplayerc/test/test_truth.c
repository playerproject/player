/***************************************************************************
 * Desc: Tests for the truth device (must run Stage)
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#include <math.h>


// Basic test for truth device.
int test_truth(playerc_client_t *client, int index)
{
  int t;
  double i_px, i_py, i_pa;
  double f_px, f_py, f_pa;
  void *rdevice;
  playerc_truth_t *device;

  printf("device [truth] index [%d]\n", index);

  device = playerc_truth_create(client, index);

  TEST("subscribing (read)");
  if (playerc_truth_subscribe(device, PLAYER_READ_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 3; t++)
  {
    TEST("getting pose (req/rep)");
    if (playerc_truth_get_pose(device, &f_px, &f_py, &f_pa) != 0)
    {
      FAIL();
      return -1;
    }
    PASS();
    printf("truth: [%6.3f] [%6.3f] [%6.3f]\n", f_px, f_py, f_pa);
  }
  
  TEST("setting pose");
  i_px = 2; i_py = 3; i_pa = M_PI/2;
  if (playerc_truth_set_pose(device, i_px, i_py, i_pa) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting pose (req/rep)");
  if (playerc_truth_get_pose(device, &f_px, &f_py, &f_pa) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  printf("truth: [%6.3f] [%6.3f] [%6.3f]\n", f_px, f_py, f_pa);

  TEST("checking values for consitency");
  if (fabs(f_px - i_px) > 0.001 || fabs(f_py - i_py) > 0.001 || fabs(f_pa - i_pa) > 0.001)
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
      printf("truth: [%6.3f] [%6.3f] [%6.3f]\n",
             device->px, device->py, device->pa);
    }
    else
      FAIL();
  }

  TEST("unsubscribing");
  if (playerc_truth_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_truth_destroy(device);
  
  return 0;
}

