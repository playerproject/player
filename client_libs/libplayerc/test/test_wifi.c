/***************************************************************************
 * Desc: Tests for the wifi device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for wifi device.
int test_wifi(playerc_client_t *client, int index)
{
  int i, t;
  double i_px, i_py, i_pa;
  double f_px, f_py, f_pa;
  void *rdevice;
  playerc_wifi_t *device;

  printf("device [wifi] index [%d]\n", index);

  device = playerc_wifi_create(client, index);

  TEST("subscribing (read)");
  if (playerc_wifi_subscribe(device, PLAYER_READ_MODE) != 0)
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

      for (i = 0; i < device->link_count; i++)
      {
        printf("wifi: [%s] [%4d] [%4d] [%4d]\n", device->links[i].ip,
               device->links[i].link, device->links[i].level, device->links[i].noise);
      }
    }
    else
      FAIL();
  }

  TEST("unsubscribing");
  if (playerc_wifi_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_wifi_destroy(device);
  
  return 0;
}

