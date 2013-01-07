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
#if !defined (WIN32)
  #include <unistd.h>
#endif

using namespace PlayerCc;

int
test_aio(PlayerClient* client, int index)
{
  TEST("aio");
  try {
    AioProxy ap(client,index);

    for(int t = 0; t < 5; t++)
    {
      TEST1("reading data (attempt %d)", t);

      client->Read();
      PASS();

      std::cerr << ap << std::endl;
    }


  } catch(std::exception& e) {
    FAIL();
    std::cerr << e.what() << std::endl;
    return -1;
  }

  PASS();
  return 0;
}
