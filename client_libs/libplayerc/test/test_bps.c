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
  int id;
  double i_px, i_py, i_pa, i_ux, i_uy, i_ua;
  double f_px, f_py, f_pa, f_ux, f_uy, f_ua;  
  void *rdevice;
  playerc_bps_t *device;

  printf("device [bps] index [%d]\n", index);

  device = playerc_bps_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_bps_subscribe(device, PLAYER_ALL_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("setting beacon pose");
  id = 212;
  i_px = 1; i_py = 0; i_pa = M_PI;
  i_ux = 0; i_uy = 0; i_ua = 0;
  if (playerc_bps_set_beacon(device, id, i_px, i_py, i_pa, i_ux, i_uy, i_ua) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting beacon pose");
  if (playerc_bps_get_beacon(device, id, &f_px, &f_py, &f_pa, &f_ux, &f_uy, &f_ua) != 0)
  {
    FAIL();
    return -1;
  }
  if (f_px != i_px || f_py != i_py || f_pa != i_pa)
  {
    FAIL();
    return -1;
  }
  if (f_ux != i_ux || f_uy != i_uy || f_ua != i_ua)
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
      printf("bps: [%6.3f] [%6.3f] [%6.3f] [%6.3f]\n",
             device->px, device->py, device->pa, device->err);
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

