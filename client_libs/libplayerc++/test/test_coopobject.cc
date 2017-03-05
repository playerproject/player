/*
 *  test_coopobject.cc : a Player client library test
 *  Copyright (C) Andrew Howard and contributors 2002-2007
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */
/***************************************************************************
 * Desc: Test for the Cooperating Object proxy
 * Author: Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/

#include "test.h"
#if !defined (WIN32) || defined (__MINGW32__)
  #include <unistd.h>
#endif

#if !HAVE_USLEEP
  #include <replace.h>
#endif

int test_coopobject(PlayerClient* client, int index)
{
  TEST("CoopObject");
  try
  {
    using namespace PlayerCc;

    CoopObjectProxy *cp = new CoopObjectProxy(client, index);

	for (int i=0; i<20; ++i) {
		usleep(500000);
		printf("trying %d ",i);
		TEST("read wsn");
		client->Read();
		PASS();
	
		std::cout << *cp << std::endl;

	}

	std::cout << "Unsuscribing for 10 seconds\n" << std::endl;

	delete cp;

	usleep(10000000);

	std::cout << "Suscribing again\n" << std::endl;

	cp = new CoopObjectProxy(client, index);

	for (int i=0; i<20; ++i) {
		usleep(500000);
		printf("trying %d ",i);
		TEST("read wsn");
		client->Read();
		PASS();
	
		std::cout << *cp << std::endl;

	}
	delete cp;
/*       
    TEST("setting the data frequency rate");
    cp.DataFreq(-1, -1, 1);
    PASS ();
    
    TEST("enabling all LEDs");
    cp.SetDevState(-1, -1, 3, 7);
    PASS();
*/
  }
  catch (PlayerCc::PlayerError e)
  {
      std::cerr << e << std::endl;
      return -1;
  }
  return 1;
}
