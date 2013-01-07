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
 * a test for the C++ CameraProxy
 */

#include "test.h"

int test_camera(PlayerClient *client, int index)
{
  TEST("camera");
  try
  {
    using namespace PlayerCc;

    CameraProxy cp(client, index);

    for (int i=0; i<10; ++i)
    {
      TEST("read camera");
      client->Read();
      PASS();

      std::cout << cp << std::endl;

      if (i>5)
      {
        TEST("save frame");
        cp.SaveFrame("test_");
        PASS();
      }
    }
  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
  return 1;
}
