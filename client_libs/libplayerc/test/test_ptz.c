/***************************************************************************
 * Desc: Tests for the PTZ device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic ptz test
int test_ptz(playerc_client_t *client, int robot, int index)
{
  int t;
  void *rdevice;
  playerc_ptz_t *device;

  printf("device [ptz] index [%d]\n", index);

  device = playerc_ptz_create(client, robot, index);

  TEST("subscribing (read)");
  if (playerc_ptz_subscribe(device, PLAYER_ALL_MODE) == 0)
    PASS();
  else
    FAIL();

  for (t = 0; t < 10; t++)
  {    
    TEST1("reading data (attempt %d)", t);
    
    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);
    
    if (rdevice == device)
    {
      PASS();
      printf("ptz: [%d %d %d]\n",
             (int) (device->pan * 180 / M_PI),
             (int) (device->tilt * 180 / M_PI),
             (int) (device->zoom * 180 / M_PI));
    }
    else
    {
      FAIL();
      break;
    }

    TEST1("writing data (attempt %d)", t);

    if (playerc_ptz_set(device, t * M_PI / 20, t * M_PI / 40, t * 100) != 0)
    {
      FAIL();
      break;
    }
    PASS();
  }
  
  TEST("unsubscribing");
  if (playerc_ptz_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_ptz_destroy(device);
  
  return 0;
}


