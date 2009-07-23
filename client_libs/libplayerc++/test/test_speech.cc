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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
 * Testing the Speech Proxy. Alexis Maldonado. May 4 2007.
 */

#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif
#include <string>

#include <playerconfig.h>
#if !HAVE_USLEEP
  #include <replace.h>
#endif

using namespace std;

using namespace PlayerCc;

int
test_speech(PlayerClient* client, int index)
{
  TEST("speech");
  SpeechProxy sp(client,index);

  TEST("speech: saying something");

  string hello("Hello World!");
  string numbers("12345678901234567890123456789012345678901234567890");


  TEST1("writing data (attempt %d)", 1);

  sp.Say(hello.c_str());

  PASS();
  usleep(1000000);

  TEST1("writing data (attempt %d)", 2);

  sp.Say(numbers.c_str());

  PASS();
  
  return(0);
}

