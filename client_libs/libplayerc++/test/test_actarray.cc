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
 * a test for the C++ SonarProxy
 */

#include "test.h"
#if !defined (WIN32) || defined (__MINGW32__)
  #include <unistd.h>
#endif

using namespace PlayerCc;

int
test_actarray(PlayerClient* client, int index)
{
  ActArrayProxy aap(client,index);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();


  aap.RequestGeometry();
  aap.GetActuatorGeom(0);

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);
    client->Read();

    std::cerr << "got actarray data: " << std::endl << aap << std::endl;

    PASS();
  }

  const int wait_iters = 50;

  TEST("homing actuator #0");
  aap.MoveHome(0);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  PASS();

  TEST("moving #0 to 1.0");
  aap.MoveTo(0, 1.0);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  PASS();

  TEST("moving #0 to 0.0");
  aap.MoveTo(0, 0.0);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  PASS();

  TEST("moving #0 to 0.5");
  aap.MoveTo(0, 0.5);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  PASS();

  TEST("moving #0 at speed 0.25, then setting speed to 0");
  aap.MoveAtSpeed(0, 0.25);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  aap.MoveAtSpeed(0, 0.0);
  PASS();

  TEST("moving #0 at speed -0.3, then setting speed to 0");
  aap.MoveAtSpeed(0, -0.3f);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  aap.MoveAtSpeed(0, 0.0);
  PASS();

  TEST("homing #0");
  aap.MoveHome(0);
  for(int i = 0; i < wait_iters; ++i)
  {
    client->Read();
    if(i % 5 == 0)
      std::cerr << aap << std::endl;
  }
  PASS();

  PASS();
  return(0);
}


 	  	 
