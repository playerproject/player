/***************************************************************************
 * Desc: Tests for the position2d device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include <libplayercore/playercommon.h>

#include "test.h"
#include "playerc.h"


// Basic test for simulation device.
int test_simulation(playerc_client_t *client, int index)
{
  double x,y,a;
  void *rdevice;
  playerc_simulation_t *device;

  printf("device [simulation] index [%d]\n", index);

  device = playerc_simulation_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_simulation_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting pose for model robot1");
  if (playerc_simulation_get_pose2d(device, "robot1", &x, &y, &a) == 0)
  {
    PASS();
    printf("pose: (%.3f, %.3f, %.3f)\n", x,y,RTOD(a));
  }
  else
    FAIL();

  TEST("setting pose for model robot1 to (0,0,0)");
  if (playerc_simulation_set_pose2d(device, "robot1", 0, 0, 0) == 0)
    PASS();
  else
    FAIL();

  puts("Sleeping...");
  sleep(3);

  TEST("returning model robot1 to original pose");
  if (playerc_simulation_set_pose2d(device, "robot1", x, y, a) == 0)
    PASS();
  else
    FAIL();
  
  TEST("setting property \"fiducial_return\" for model robot1 to 42");
  int fr = 42;
  if (playerc_simulation_set_property(device, "robot1", "_mp_fiducial_return", &fr, sizeof(fr) ) == 0)
    PASS();
  else
    FAIL();
  
  int col =  0xFF00;
  TEST("setting integer property \"color\" for model robot1 to 0x00FF00 (green)");
  if (playerc_simulation_set_property(device, "robot1", "_mp_color", &col, sizeof(col) ) == 0)
    PASS();
  else
    FAIL();
  
  sleep(1);
  
  col = 0xFF;
  TEST("setting integer property \"color\" for model robot1 to 0x0000FF (blue)");
  if (playerc_simulation_set_property(device, "robot1", "_mp_color", &col, sizeof(col)) == 0)
    PASS();
  else
    FAIL();
  
  sleep(1);

  col = 0xFF0000;
  TEST("setting integer property \"color\" for model robot1 to 0xFF0000 (red)");
  if (playerc_simulation_set_property(device, "robot1", "_mp_color", &col, sizeof(col)) == 0)
    PASS();
  else
    FAIL();
  
  TEST("unsubscribing");
  if (playerc_simulation_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_simulation_destroy(device);
  
  return 0;
}

