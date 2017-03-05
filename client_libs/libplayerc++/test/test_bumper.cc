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
 * a test for the C++ BumperProxy
 */

#include "test.h"
#if !defined (WIN32)
  #include <unistd.h>
#endif

int
test_bumper(PlayerClient* client, int index)
{
  TEST("bumper");
  BumperProxy sp(client,index);

  // wait for P2OS to start up
  for(int i=0; i < 20; i++)
    client->Read();

  /*
  TEST("getting bumper geometry");

  player_bumper_geom_t bumper_geom;
  sp.GetBumperGeom( &bumper_geom );
  sleep(1);
  PASS();
  printf( "Discovered %d bumper geometries\n", bumper_geom.bumper_count );
  for(int i=0;i<bumper_geom.bumper_count;i++)
    {
    printf("Bumper[%d]: (%4d,%4d,%4d) len: %4u radius: %4u\n", i, 
           bumper_geom.bumper_def[i].x_offset,
           bumper_geom.bumper_def[i].y_offset,
           bumper_geom.bumper_def[i].th_offset,
           bumper_geom.bumper_def[i].length,
           bumper_geom.bumper_def[i].radius );
  }
  */

  for(int t = 0; t < 3; t++)
  {
    TEST1("reading data (attempt %d)", t);

    client->Read();

    PASS();

    std::cerr << sp << std::endl;
    if(sp.IsAnyBumped()) {
      std::cerr << "A bumper switch is pressed.\n";
    }
  }


  PASS();

  return(0);
}

