/***************************************************************************
 * Desc: Tests for the position3d device
 * Author: Andrew Howard
 * Date: 23 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


// Basic test for position3d device.
int test_position3d(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_position3d_t *device;

  printf("device [position3d] index [%d]\n", index);

  device = playerc_position3d_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_position3d_subscribe(device, PLAYER_ALL_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  TEST("getting geometry");
  if (playerc_position3d_get_geom(device) == 0)
    PASS();
  else
    FAIL();
  printf("position3d geom: [%6.3f %6.3f %6.3f] [%6.3f %6.3f]\n",
         device->pose[0], device->pose[1], device->pose[2], device->size[0], device->size[1]);
  
  for (t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();
      printf("position3d: [%14.3f] [%+7.3f %+7.3f %+7.3f] [%+7.3f %+7.3f %+7.3f] [%d]\n",
             device->info.datatime,
             device->pos_x, device->pos_y, device->pos_z,
             device->pos_roll * 180/M_PI, device->pos_pitch * 180/M_PI, device->pos_yaw * 180/M_PI,
             device->stall);
    }
    else
    {
      //printf("error: %s", playerc_error_str());
      FAIL();
      break;
    }
  }
  
  TEST("unsubscribing");
  if (playerc_position3d_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_position3d_destroy(device);
  
  return 0;
}

