/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>

int
test_position(PlayerClient* client, int index)
{
  unsigned char access;
  PositionProxy pp(client,index);

  printf("device [position] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((pp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", pp.driver_name);
    return -1;
  }
  PASS();

  printf("DRIVER: %s\n", pp.driver_name);

  // wait for P2OS to start up
  for(int i=0;i<20;i++)
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

    pp.Print();
  }

  TEST("enabling motors");
  if(pp.SetMotorState(1) < 0)
  {
    FAIL();
    return(-1);
  }
  PASS();

 //   TEST("moving forward");
//    if(pp.SetSpeed(100,0) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

//    TEST("moving backward");
//    if(pp.SetSpeed(-100,0) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

//    TEST("moving left");
//    if(pp.SetSpeed(0,100,0) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

//    TEST("moving right");
//    if(pp.SetSpeed(0,-100,0) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

//    TEST("turning right");
//    if(pp.SetSpeed(0,-25) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

//    TEST("turning left");
//    if(pp.SetSpeed(0,25) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

//    TEST("stopping");
//    if(pp.SetSpeed(0,0) < 0)
//    {
//      FAIL();
//      return(-1);
//    }
//    sleep(3);
//    PASS();

  TEST("changing to position control mode");
  if(pp.SelectPositionMode(1) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("resetting odometry");
  if(pp.ResetOdometry() < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("moving to [1000 0 0]");
  if(pp.GoTo( 1000, 0, 0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(5);
  PASS();

  getchar();

  TEST("moving to [1000 0 -90]");
  if(pp.GoTo( 1000, 0, -90) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(5);
  PASS();

  getchar();


  TEST("moving to [0.0 0.0 0.0]");
  if(pp.GoTo( 0, 0, 0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(5);
  PASS();
      
  getchar();

  TEST("changing back to velocity control mode");
  if(pp.SelectPositionMode(0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();


  TEST("disabling motors");
  if(pp.SetMotorState(0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("changing to separate velocity control");
  if(pp.SelectVelocityControl(1) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("changing to direct wheel velocity control");
  if(pp.SelectVelocityControl(0) < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("resetting odometry");
  if(pp.ResetOdometry() < 0)
  {
    FAIL();
    return(-1);
  }
  sleep(1);
  PASS();

  TEST("unsubscribing");
  if((pp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

