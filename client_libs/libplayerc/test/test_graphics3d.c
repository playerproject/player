/***************************************************************************
 * Desc: Tests for the graphics3d device
 * Author: Richard Vaughan
 * Date: 15 June 2007
 # CVS: $Id$
 **************************************************************************/

#include <unistd.h>
#include <math.h>

#include "test.h"
#include "playerc.h"

#define RAYS 64

// Basic test for graphics3d device.
int test_graphics3d(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_graphics3d_t *device;

  printf("device [graphics3d] index [%d]\n", index);

  device = playerc_graphics3d_create(client, index);

  TEST("subscribing (read/write)");
  if (playerc_graphics3d_subscribe(device, PLAYER_OPEN_MODE) < 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  
  player_point_3d_t pts[RAYS];
  
  double r;
  for( r=0; r<1.0; r+=0.05 )
    {
      TEST("drawing points");
      
      int p;
      for( p=0; p<RAYS; p++ )
	{
	  pts[p].px = 100 * r * cos(p * M_PI/(RAYS/2));
	  pts[p].py = 100 * r * sin(p * M_PI/(RAYS/2));
	  pts[p].pz = 0;

	  printf( "vertex [%.2f,%.2f,%.2f]\n",
		  pts[p].px,
		  pts[p].py,
		  pts[p].pz );
	}	
      
      if( playerc_graphics3d_draw(device, 
				  PLAYER_DRAW_POINTS, 
				  pts, RAYS) < 0)
	FAIL();
      else
	PASS();

      usleep(100000);
    }
  
  TEST("changing color");
  player_color_t col;
  col.red = 0;
  col.green = 255;
  col.blue = 0;
  col.alpha = 0;

  if(playerc_graphics3d_setcolor(device, col) < 0)
    FAIL();
  else
    PASS();

  TEST("drawing polyline");
  
  if(playerc_graphics3d_draw(device, 
			     PLAYER_DRAW_LINE_LOOP,
			     pts, RAYS) < 0)
    FAIL();
  else
    PASS();
  
  usleep(500000);
  
  TEST("changing color");      
  col.red = 0;
  col.green = 128;
  col.blue = 255;
  col.alpha = 0;
  
  if(playerc_graphics3d_setcolor(device, col) < 0)
    FAIL();
  else
    PASS();

  for( r=1.0; r>0; r-=0.1 )
    {
      TEST("drawing polygon");
      
      player_point_3d_t pts[4];
      pts[0].px = -r;
      pts[0].py = -r;
      pts[0].pz = 0;
      pts[1].px = r;
      pts[1].py = -r;
      pts[1].pz = 0;
      pts[2].px = r;
      pts[2].py = r;
      pts[2].pz = 0;
      pts[3].px = -r;
      pts[3].py = r;
      pts[3].pz = 0;
      
      if(playerc_graphics3d_draw(device, 
				 PLAYER_DRAW_POLYGON,
				 pts, 4 ) < 0)
	FAIL();
      else
	PASS();
      
      usleep(100000);
    }

  sleep(2);


  TEST("clearing");
  
  if(playerc_graphics3d_clear(device) < 0)
    FAIL();
  else
    PASS();


  TEST("unsubscribing");
  if (playerc_graphics3d_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_graphics3d_destroy(device);
  
  return 0;
}

