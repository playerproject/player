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
 * a test for the player robot nameservice
 */

#include "test.h"

int
test_lookup(PlayerClient* client, int index)
{
  char robotname[] = "robot0";
  int port;

  printf("nameservice test, using robot \"%s\"\n", robotname);
  TEST("looking up port");
  port = client->LookupPort(robotname);
  if(port < 0)
  {
    printf("the request failed");
    FAIL();
    return -1;
  }
  else if(port == 0)
  {
    printf("couldn't find the indicated robot");
    FAIL();
    return -1;
  }
  printf("got port %d\n", port);
  PASS();

  return(0);
}

