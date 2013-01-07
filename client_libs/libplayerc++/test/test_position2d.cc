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
 * a test for the C++ Position2DProxy
 */

#include "test.h"
#if !defined (WIN32) || defined (__MINGW32__)
  #include <unistd.h>
#endif
#include <math.h>

#include <playerconfig.h>
#if !HAVE_USLEEP
  #include <replace.h>
#endif

using namespace PlayerCc;

int
test_position2d(PlayerClient* client, int index)
{
  TEST("position2d");
  Position2dProxy p2d(client,index);

  // wait for P2OS to start up
  for(int i=0;i<20;i++)
    client->Read();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();

    std::cerr << p2d << std::endl;
  }

  const double ox = 0.1, oy = -0.2;
  const int oa = 180;
  
  TEST("Setting odometry" );
  p2d.SetOdometry(ox, oy, DTOR((double)oa));

  printf("\n - initial \t[%.3f %.3f %.3f]\n"
   " - requested \t[%.3f %.3f %.3f]\n", 
   p2d.GetXPos(), p2d.GetYPos(), RTOD(p2d.GetYaw()), 
   ox, oy, (double)oa);
  
  
  for( int s=0; s<10; s++ )
  {
    client->Read();
    printf( " - reading \t[%.3f %.3f %.3f]\r", 
      p2d.GetXPos(), p2d.GetYPos(), RTOD(p2d.GetYaw()) );
    fflush(stdout);
  }

  puts("");
  
  if((p2d.GetXPos() != ox) || 
     (p2d.GetYPos() != oy) || 
#if defined (WIN32)
     ((int)round(RTOD(p2d.GetYaw())) != oa))
#else
     ((int)rint(RTOD(p2d.GetYaw())) != oa))
#endif
  {
    FAIL();
    //return(-1);
  }
  else
  {
    PASS();
  }

  TEST("resetting odometry");
  p2d.ResetOdometry();
  usleep(1000000);
  PASS();

  TEST("enabling motors");
  p2d.SetMotorEnable(1);
  PASS();

  TEST("moving forward");
  p2d.SetSpeed(0.1,0);
  usleep(3000000);
  PASS();
  
  TEST("moving backward");
  p2d.SetSpeed(-0.1,0);
  usleep(3000000);
  PASS();
  
  TEST("moving left");
  p2d.SetSpeed(0,0.1,0);
  usleep(3000000);
  PASS();
  
  TEST("moving right");
  p2d.SetSpeed(0,-0.1,0);
  usleep(3000000);
  PASS();
  
  TEST("turning right");
  p2d.SetSpeed(0,DTOR(-25.0));
  usleep(3000000);
  PASS();

  TEST("turning left");
  p2d.SetSpeed(0,DTOR(25.0));
  usleep(3000000);
  PASS();

  TEST("moving left and anticlockwise (testing omnidrive)");
  p2d.SetSpeed( 0, 0.1, DTOR(45.0) );
  usleep(3000000);
  PASS();
  
  
  TEST("moving right and clockwise (testing omnidrive)");
  p2d.SetSpeed( 0, -0.1, DTOR(-45) );
  usleep(3000000);
  PASS();
  
  TEST("stopping");
  p2d.SetSpeed(0,0);
  usleep(3000000);
  PASS();


  TEST("disabling motors");
  p2d.SetMotorEnable(0);
  usleep(1000000);
  PASS();
  
  /*
  TEST("changing to separate velocity control");
  p2d.SelectVelocityControl(1);
  sleep(1);
  PASS();
  
  TEST("changing to direct wheel velocity control");
  p2d.SelectVelocityControl(0);
      sleep(1);
      PASS();
  */
  
  TEST("resetting odometry");
  p2d.ResetOdometry();
      usleep(1000000);
      PASS();
    
  
  PASS();
  return 0;
}

