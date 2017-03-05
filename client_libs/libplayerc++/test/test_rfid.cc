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
 * a test for the C++ RFIDProxy
 */

#include "test.h"

int test_rfid(PlayerClient* client, int index)
{
  TEST("rfid");
  try
  {
	  using namespace PlayerCc;

	  RFIDProxy cp(client, index);

      // wait for the rfid to warm up
	  for(int i=0;i<20;i++)
		  client->Read();

	  for (int i=0; i<10; ++i)
	  {
		  TEST("read rfid");
		  client->Read();
		  PASS();

		  std::cout << cp << std::endl;
	   }
  }
  catch (PlayerCc::PlayerError e)
  {
	  std::cerr << e << std::endl;
	  return -1;
  }
  return 1;
}
