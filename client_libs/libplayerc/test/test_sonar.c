/***************************************************************************
 * Desc: Tests for the sonar device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic sonar test
int test_sonar(playerc_client_t *client, int index)
{
  int t, i;
  void *rdevice;
  playerc_sonar_t *device;

  printf("device [sonar] index [%d]\n", index);

  device = playerc_sonar_create(client, index);

  TEST("subscribing (read)");
  if (playerc_sonar_subscribe(device, PLAYER_READ_MODE) == 0)
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
      printf("sonar: [%d] ", device->scan_count);
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
  if (playerc_sonar_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_sonar_destroy(device);
  
  return 0;
}


