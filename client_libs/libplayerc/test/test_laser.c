/***************************************************************************
 * Desc: Tests for the laser device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic laser test
int test_laser(playerc_client_t *client, int index)
{
  int t, i;
  playerc_laser_t *laser;

  double min, max;
  int resolution, intensity;

  printf("device [laser] index [%d]\n", index);

  laser = playerc_laser_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_laser_subscribe(laser, PLAYER_ALL_MODE) == 0)
    PASS();
  else
    FAIL();
  
  TEST("set configuration");
  min = -M_PI/2;
  max = +M_PI/2;
  resolution = 100;
  intensity = 1;
  if (playerc_laser_set_config(laser, min, max, resolution, intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("get configuration");
  if (playerc_laser_get_config(laser, &min, &max, &resolution, &intensity) == 0)
    PASS();
  else
    FAIL();

  TEST("check configuration sanity");
  if (abs(min + M_PI/2) > 0.01 || abs(max - M_PI/2) > 0.01)
    FAIL();
  else if (resolution != 100 || intensity != 1)
    FAIL();
  else
    PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);
    if (playerc_client_read(client) != 0)
    {
      FAIL();
      break;
    }
    PASS();

    printf("laser: [%d] ", laser->scan_count);
    for (i = 0; i < 3; i++)
      printf("[%6.3f, %6.3f] ", laser->scan[i][0], laser->scan[i][1]);
    printf("\n");
  }
  
  TEST("unsubscribing");
  if (playerc_laser_unsubscribe(laser) == 0)
    PASS();
  else
    FAIL();
  
  playerc_laser_destroy(laser);
  
  return 0;
}


