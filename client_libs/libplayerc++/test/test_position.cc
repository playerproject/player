/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */
/*
 * $Id$
 *
 * a test for the C++ PositionProxy
 */

#include "test.h"
#include <unistd.h>
#include <math.h>

int
test_position(PlayerClient* client, int index)
{
  unsigned char access;
  PositionProxy pp(client,index,'c');

  printf("device [position] index [%d]\n", index);

  TEST("subscribing (read/write)");
  if((pp.ChangeAccess(PLAYER_ALL_MODE,&access) < 0) ||
     (access != PLAYER_ALL_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", pp.driver_name);
    printf("access:%d\n", access);
    return -1;
  }
  PASS();

  printf("DRIVER: %s\n", pp.driver_name);

  // wait for P2OS to start up
  for(int i=0;i<20;i++)
    client->Read();

  TEST("getting geometry");
  if(pp.GetGeometry() < 0)
    FAIL();
  else
  {
    printf("size: [%.3lf %.3lf] pose: [%.3lf %.3lf %.3lf]\n",
           pp.size[0], pp.size[1],
           pp.pose[0], pp.pose[1], RTOD(pp.pose[2]));
    PASS();
  }

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

  const double ox = 0.1, oy = -0.2;
  const double oa = 6.21569;
  
  TEST("Setting odometry" );
  if( pp.SetOdometry(ox, oy, oa) < 0 )
    {
      FAIL();
      //return(-1);
    }
  else
    {
      printf("\n - initial \t[%.3f %.3f %.3f]\n"
	     " - requested \t[%.3f %.3f %.3f]\n", 
	     pp.xpos, pp.ypos, RTOD(pp.theta), 
	     ox, oy, RTOD(oa));
      
      
      for( int s=0; s<10; s++ )
	{
	  client->Read();
	  printf( " - reading \t[%.3f %.3f %.3f]\r", 
		  pp.xpos, pp.ypos, RTOD(pp.theta) );
	  fflush(stdout);
	}
  
      puts("");
      
      if((pp.xpos != ox) || 
         (pp.ypos != oy) || 
         (pp.theta != oa))
	{
	  FAIL();
	  //return(-1);
	}
      else
	PASS();
    }
  
  TEST("resetting odometry");
  if(pp.ResetOdometry() < 0)
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
  if(pp.SetMotorState(1) < 0)
  {
    FAIL();
    //return(-1);
  }
  else
    PASS();

  TEST("moving forward");
  if(pp.SetSpeed(0.1,0) < 0)
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
  if(pp.SetSpeed(-0.1,0) < 0)
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
  if(pp.SetSpeed(0,0.1,0) < 0)
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
  if(pp.SetSpeed(0,-0.1,0) < 0)
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
  if(pp.SetSpeed(0,DTOR(-25.0)) < 0)
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
  if(pp.SetSpeed(0,DTOR(25.0)) < 0)
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
  if( pp.SetSpeed( 0, 0.1, DTOR(45.0) ) < 0 )
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
  if( pp.SetSpeed( 0, -0.1, DTOR(-45) ) < 0 )
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
  if(pp.SetSpeed(0,0) < 0)
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
  if(pp.SetMotorState(0) < 0)
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
  if(pp.SelectVelocityControl(1) < 0)
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
  if(pp.SelectVelocityControl(0) < 0)
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
  if(pp.ResetOdometry() < 0)
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
  if((pp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
    {
      FAIL();
      return -1;
    }

  PASS();
  
  return(0);
}

