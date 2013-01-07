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
 * a test for the C++ LocalizeProxy
 */

#include "test.h"
#include <unistd.h>

int
test_localize(PlayerClient* client, int index)
{
  unsigned char access;
  LocalizeProxy lp(client,index,'c');
  int t;
  int num_particles;
  double pose[3] = {0.0,0.0,0.0};
  double cov[3][3] = {{0.0,0.0,0.0},
                      {0.0,0.0,0.0},
                      {0.0,0.0,0.0}};

  printf("device [localize] index [%d]\n", index);

  TEST("subscribing (read)");
  if((lp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", lp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", lp.driver_name);

  TEST("waiting for the localization system to start up");
  for(t = 0; t < 100; t++)
  {
    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }
    if(lp.hypoth_count)
      break;
  }
  // did we timeout?
  if(t == 100)
  {
    FAIL();
    return(-1);
  }
  PASS();

  TEST("setting the pose");
  if((lp.SetPose(pose,cov) < 0) || (client->Read() < 0))
  {
    FAIL();
    return(-1);
  }
  PASS();

  TEST("getting the number of particles");
  if((num_particles = lp.GetNumParticles()) < 0)
  {
    FAIL();
    return(-1);
  }
  printf("%d  ",num_particles);
  PASS();

// deprecated: get map from map interface instead
#if 0
  TEST("getting the map");
  if((num_particles = lp.GetMap()) < 0)
  {
    FAIL();
    return(-1);
  }
  PASS();
#endif

  for(t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    if(client->Read() < 0)
    {
      FAIL();
      return(-1);
    }

    PASS();

    lp.Print();
  }

  TEST("unsubscribing");
  if((lp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

