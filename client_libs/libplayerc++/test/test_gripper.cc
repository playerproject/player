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

#include <playerconfig.h>
#if !HAVE_USLEEP
  #include <replace.h>
#endif

using namespace PlayerCc;

int
test_gripper(PlayerClient* client, int index)
{
  GripperProxy gp(client,index);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);
    client->Read();

    std::cerr << "got gripper data: " << std::endl << gp << std::endl;

    PASS();
  }

  TEST("gripper open");
  gp.Open();
  usleep(5000000);
  PASS();

  TEST("gripper close");
  gp.Close();
  usleep(8000000);
  PASS();

  TEST("gripper open");
  gp.Open();
  usleep(5000000);
  PASS();

  TEST("gripper store object (only on some grippers, e.g. stage)");
  gp.Store();
  usleep(3000000);
  PASS();

  TEST("gripper retrieve object (only on some grippers, e.g. stage");
  gp.Retrieve();
  usleep(3000000);
  PASS();

  PASS();
  return(0);
}

