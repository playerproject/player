/*
 * $Id$
 *
 * a test for the C++ SonarProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_gripper(PlayerClient* client, int index)
{
  unsigned char access;
  GripperProxy gp(client,index,'c');

  printf("device [gripper] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((gp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", gp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", gp.driver_name);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    gp.Print();
  }

  TEST("gripper deploy");
  if(gp.SetGrip(GRIPdeploy) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(3);
  PASS();

  TEST("gripper store");
  if(gp.SetGrip(GRIPstore) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(4);
  PASS();

  TEST("unsubscribing");
  if((gp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

