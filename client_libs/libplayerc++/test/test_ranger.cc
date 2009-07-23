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
#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif

int
test_ranger(PlayerClient* client, int index)
{
  TEST("ranger");
  RangerProxy rp(client,index);

  // wait for P2OS to start up, throwing away data as fast as possible
  for(int i=0; i < 60; i++)
  {
    client->Read();
  }

  rp.RequestGeom();

  std::cout << "There are " << rp.GetElementCount() << " individual range sensors.\n";

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();
    std::cout << rp << std::endl;

  }

  PASS();
  return 0;
}

