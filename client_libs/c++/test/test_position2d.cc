/*
 * $Id$
 *
 * a test for the C++ Position2DProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>
#include <math.h>

int
test_position2d(PlayerClient* client, int index)
{
  unsigned char access;
  Position2DProxy p2d(client,index,'c');

  printf("device [position] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((p2d.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", p2d.driver_name);
    printf("access:%d\n", access);
    return -1;
  }
  PASS();

  printf("DRIVER: %s\n", p2d.driver_name);

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

    p2d.Print();
  }

  const double ox = 0.1, oy = -0.2;
  const int oa = 180;
  
  TEST("Setting odometry" );
  if( p2d.SetOdometry(ox, oy, DTOR((double)oa)) < 0 )
    {
      FAIL();
      //return(-1);
    }
  else
    {
      printf("\n - initial \t[%.3f %.3f %.3f]\n"
	     " - requested \t[%.3f %.3f %.3f]\n", 
	     p2d.Xpos(), p2d.Ypos(), RTOD(p2d.Yaw()), 
	     ox, oy, (double)oa);
      
      
      for( int s=0; s<10; s++ )
	{
	  client->Read();
	  printf( " - reading \t[%.3f %.3f %.3f]\r", 
		  p2d.Xpos(), p2d.Ypos(), RTOD(p2d.Yaw()) );
	  fflush(stdout);
	}
  
      puts("");
      
      if((p2d.Xpos() != ox) || 
         (p2d.Ypos() != oy) || 
         ((int)rint(RTOD(p2d.Yaw())) != oa))
	{
	  FAIL();
	  //return(-1);
	}
      else
	PASS();
    }
  
  TEST("resetting odometry");
  if(p2d.ResetOdometry() < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    {
      sleep(1);
      PASS();
    }

  TEST("enabling motors");
  if(p2d.SetMotorState(1) < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    PASS();

  TEST("moving forward");
  if(p2d.SetSpeed(0.1,0) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }
  
  TEST("moving backward");
  if(p2d.SetSpeed(-0.1,0) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }
  
  TEST("moving left");
  if(p2d.SetSpeed(0,0.1,0) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }
  
  TEST("moving right");
  if(p2d.SetSpeed(0,-0.1,0) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }
  
  TEST("turning right");
  if(p2d.SetSpeed(0,DTOR(-25.0)) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }

  TEST("turning left");
  if(p2d.SetSpeed(0,DTOR(25.0)) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }

  TEST("moving left and anticlockwise (testing omnidrive)");
  if( p2d.SetSpeed( 0, 0.1, DTOR(45.0) ) < 0 )
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }
  
  
  TEST("moving right and clockwise (testing omnidrive)");
  if( p2d.SetSpeed( 0, -0.1, DTOR(-45) ) < 0 )
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }
  
  TEST("stopping");
  if(p2d.SetSpeed(0,0) < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(3);
      PASS();
    }


  TEST("disabling motors");
  if(p2d.SetMotorState(0) < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    {
      sleep(1);
      PASS();
    }
  
  TEST("changing to separate velocity control");
  if(p2d.SelectVelocityControl(1) < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    { 
      sleep(1);
      PASS();
    }
  
  TEST("changing to direct wheel velocity control");
  if(p2d.SelectVelocityControl(0) < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    {
      sleep(1);
      PASS();
    }
  
  TEST("resetting odometry");
  if(p2d.ResetOdometry() < 0)
    {
      FAIL();
      //return(-1);
    }
  else
    {
      sleep(1);
      PASS();
    }
  
  TEST("unsubscribing");
  if((p2d.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
    {
      FAIL();
      return -1;
    }

  PASS();
  
  return(0);
}

