/***************************************************************************
 * Desc: Tests for the position device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for position device.
int test_position(playerc_client_t *client, int index)
{
  int t;
  playerc_position_t *position;

  printf("device [position] index [%d]\n", index);

  position = playerc_position_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_position_subscribe(position, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      return -1;
    }
    PASS();

    printf("position: [%6.3f] [%6.3f] [%6.3f] [%d]\n",
           position->px, position->py, position->pa, position->stall);
  }
  
  TEST("unsubscribing");
  if (playerc_position_unsubscribe(position) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_position_destroy(position);
  
  return 0;
}

