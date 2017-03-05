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
#if !defined (WIN32)
  #include <unistd.h>
#endif

int
test_sonar(PlayerClient* client, int index)
{
  TEST("sonar");
  SonarProxy sp(client,index);

  // wait for P2OS to start up, throwing away data as fast as possible
  for(int i=0; i < 60; i++)
  {
    client->Read();
  }

  sp.RequestGeom();

  /*
  for(int i=0;i<sp.pose_count;i++)
  {
    printf("Sonar[%d]: (%.3f,%.3f,%.3f)\n", i, 
           sp.poses[i][0],
           sp.poses[i][1],
           RTOD(sp.poses[i][2]));
  }
  */

  std::cout << "There are " << sp.GetCount() << " sonar sensors.\n";

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();
    std::cout << sp << std::endl;

  }

  PASS();
  return 0;
}

