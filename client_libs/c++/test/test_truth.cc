/*
 * $Id$
 *
 * a test for the C++ TpsProxy
 */

#include "playerclient.h"
#include "test.h"

int
test_truth(PlayerClient* client, int index)
{
  unsigned char access;
  TruthProxy tp(client,index);

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
      rx = tp.x;
      ry = tp.y;
      rth = tp.a;
    }
  

  TEST("reading config");

  double cx=0, cy=0, cth=0;
  if(tp.GetPose( &cx, &cy, &cth) < 0)
  {
    FAIL();
    return(-1);

    printf( "config reply says device is at (%.3f,%.2f,%.2f)\n",
	    cx, cy, cth );

  }
  PASS();

  TEST("comparing data pose and config pose");
  if( cx != rx || cy != ry || cth!= rth )
    FAIL();
  else
    PASS();


  TEST("teleporting around");
  for( double os = 0; os < M_PI; os += M_PI/16.0 )
    if(tp.SetPose(os,os,2.0*os) < 0)
      {
	FAIL();
	return(-1);
      }
  PASS();
  
  TEST("returning to start position");
  if(tp.SetPose(cx,cy,cth) < 0)
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

