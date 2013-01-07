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
 * a test for the C++ TpsProxy
 */

#include "test.h"

int
test_truth(PlayerClient* client, int index)
{
  unsigned char access;
  TruthProxy tp(client,index,'c');

  printf("device [truth] index [%d]\n", index);

  TEST("subscribing (read)");
  if((tp.ChangeAccess(PLAYER_READ_MODE,&access) < 0) ||
     (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", tp.driver_name);
    return -1;
  }
  PASS();
  printf("DRIVER: %s\n", tp.driver_name);

  double rx=0, ry=0, rth=0;
  
  for(int t = 0; t < 3; t++)
    {
      TEST1("reading data (attempt %d)", t);
      
      if(client->Read() < 0)
	{
	  FAIL();
	  return(-1);
	}
      
      PASS();
      
      tp.Print();
      
      // store the current pose for comparison and replacing the device
      rx = tp.px;
      ry = tp.py;
      rth = tp.rz;
    }
  

  TEST("reading config");

  double cpx=0, cpy=0, cpz=0;
  double crx=0, cry=0, crz=0;
  
  if(tp.GetPose( &cpx, &cpy, &cpz, &crx, &cry, &crz ) < 0)
  {
    FAIL();
    return(-1);

    printf( "config reply says device is at (%.3f,%.2f,%.2f)\n",
            cpx, cpy, crz );

  }
  PASS();

  TEST("comparing data pose and config pose");
  if( cpx != rx || cpy != ry || crz!= rth )
    FAIL();
  else
    PASS();


  TEST("teleporting around");
  for( double os = 0; os < M_PI; os += M_PI/16.0 )
    if(tp.SetPose(os,os,0,0,0,2.0*os) < 0)
      {
	FAIL();
	return(-1);
      }
  PASS();
  
  TEST("returning to start position");
  if(tp.SetPose(cpx,cpy,cpz,crx,cry,crz) < 0)
    {
      FAIL();
      return(-1);
    }
  PASS();


  int16_t id = 0;
  int16_t newid = 42;

  TEST("getting the original fiducial ID");
  if(tp.GetFiducialID( &id ) < 0)
    {
      FAIL();
      return(-1);
    }
  printf( "original fiducial id: %d  ", id );
  PASS();
  
  TEST("setting the fiducial ID to 42");
  if(tp.SetFiducialID( newid ) < 0)
    {
      FAIL();
      return(-1);
    }
  PASS();

  TEST("getting the new fiducial ID");
  if(tp.GetFiducialID( &id ) < 0)
    {
      FAIL();
      return(-1);
    }
  printf( "new fiducial id: %d  ", id );
  PASS();
  
  TEST("resetting fiducial ID to original value");
  if(tp.SetFiducialID( id ) < 0)
    {
      FAIL();
      return(-1);
    }
  PASS();

  TEST("unsubscribing (read)");
  if((tp.ChangeAccess(PLAYER_CLOSE_MODE,&access) < 0) ||
     (access != PLAYER_CLOSE_MODE))
  {
    FAIL();
    return -1;
  }

  PASS();

  return(0);
}

