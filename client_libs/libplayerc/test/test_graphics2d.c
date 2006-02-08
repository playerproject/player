/***************************************************************************
 * Desc: Tests for the graphics2d device
 * Author: Richard Vaughan
 * Date: 8 Feb 2006
 # CVS: $Id$
 **************************************************************************/

#include <unistd.h>

#include "test.h"
#include "playerc.h"


// Basic test for graphics2d device.
int test_graphics2d(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_graphics2d_t *device;

  printf("device [graphics2d] index [%d]\n", index);

  device = playerc_graphics2d_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_graphics2d_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("drawing points");

  player_point_2d_t pts[32];
  int p;
  for( p=0; p<32; p++ )
    {
      pts[p].px = 3.0 * cos(p * M_PI/16.0);
      pts[p].py = 3.0 * sin(p * M_PI/16.0);
    }	

  if(playerc_graphics2d_draw_points(device, pts, 32) < 0)
    FAIL();
  else
    PASS();
  

  TEST("unsubscribing");
  if (playerc_graphics2d_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_graphics2d_destroy(device);
  
  return 0;
}

