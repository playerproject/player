/***************************************************************************
 * Desc: Tests for the vision device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Basic vision test
int test_vision(playerc_client_t *client, int index)
{
  int t, i;
  playerc_vision_t *device;

  printf("device [vision] index [%d]\n", index);

  device = playerc_vision_create(client, index);

  TEST("subscribing (read)");
  if (playerc_vision_subscribe(device, PLAYER_READ_MODE) == 0)
    PASS();
  else
    FAIL();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      break;
    }
    PASS();

    printf("vision: [%d] ", device->blob_count);
    for (i = 0; i < MIN(3, device->blob_count); i++)
      printf("[%d %d %d %d] ", device->blobs[i].channel, device->blobs[i].x,
             device->blobs[i].y, device->blobs[i].area);
    printf("\n");
  }
  
  TEST("unsubscribing");
  if (playerc_vision_unsubscribe(device) == 0)
    PASS();
  else
    FAIL();
  
  playerc_vision_destroy(device);
  
  return 0;
}


