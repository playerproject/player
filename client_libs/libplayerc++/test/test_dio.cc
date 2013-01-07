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
#if !defined (WIN32) || defined (__MINGW32__)
  #include <unistd.h>
#endif

#include <playerconfig.h>
#if !HAVE_USLEEP
  #include <replace.h>
#endif

using namespace PlayerCc;

int
test_dio(PlayerClient* client, int index)
{
  TEST("dio");
  DioProxy dp(client,index);

  for(int t = 0; t < 5; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();

    std::cerr << dp << std::endl;
  }

  PASS();


  TEST("dio: setting outputs");
  unsigned int value(0);
  const unsigned int do_count(8);

  for(int t = 0; t < 5; t++)
  {
    TEST1("writing data (attempt %d)", t);
    TEST1("value: %d", value);

    dp.SetOutput(do_count,value);
    ++value;

    PASS();
    usleep(200000);
  }

  PASS();

  //turn all outputs off
  value=0;
  dp.SetOutput(do_count,value);
  
  
  return(0);
}

