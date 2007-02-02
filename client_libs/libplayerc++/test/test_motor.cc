/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "test.h"
#include <unistd.h>
#include <math.h>

int
test_motor(PlayerClient* client, int index)
{
  unsigned char access;
  MotorProxy mp(client,index,'c');

  printf("device [motor] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((mp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", mp.driver_name);
    printf("access:%d\n", access);
    return -1;
  }
  PASS();

  printf("DRIVER: %s\n", mp.driver_name);

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

    mp.Print();
  }

  const double ot = DTOR(180);
  
  TEST("Setting odometry" );
  if( mp.SetOdometry(ot) < 0 )
    {
      FAIL();
      //return(-1);
    }
  else
    {
      printf("\n - initial \t[%.3f]\n"
	     " - requested \t[%.3f]\n", 
	     RTOD(mp.Theta()), 
	     RTOD(ot));
      
      
      for( int s=0; s<10; s++ )
	{
	  client->Read();
	  printf( " - reading \t[%.3f]\r", 
		  RTOD(mp.Theta()) );
	  fflush(stdout);
	}
  
      puts("");
      
      if(ot!=mp.Theta())
	{
	  FAIL();
	  //return(-1);
	}
      else
	PASS();
    }
  
  TEST("resetting odometry");
  if(mp.ResetOdometry() < 0)
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
  if(mp.SetMotorState(1) < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    PASS();

  TEST("moving forward");
  if(mp.SetSpeed(0.1) < 0)
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
  if(mp.SetSpeed(-0.1) < 0)
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
  if(mp.SetSpeed(0) < 0)
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
  if(mp.SetMotorState(0) < 0)
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
  if((mp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
    {
      FAIL();
      return -1;
    }

  PASS();
  
  return(0);
}

