/*
 * laserobstacleavoid.cc
 *
 * a simple obstacle avoidance demo
 *
 * @todo: this has been ported to libplayerc++, but not tested
 */

#include <libplayerc++/playerc++.h>
#include <iostream>

#include "args.h"

#define RAYS 32

int
main(int argc, char **argv)
{
  parse_args(argc,argv);

  using namespace PlayerCc;

    PlayerClient robot(gHostname, gPort);
    Graphics2dProxy gp(&robot, gIndex);
    
    std::cout << robot << std::endl;
    
    double r;
    for( r=0; r<1.0; r+=0.05 )
      {
	player_point_2d_t pts[RAYS];
	int p;
	for( p=0; p<RAYS; p++ )
	  {
	    pts[p].px = r * cos(p * M_PI/(RAYS/2));
	    pts[p].py = r * sin(p * M_PI/(RAYS/2));
	  }	
	
	gp.DrawPoints( pts, RAYS );

	usleep(100000);
      }
    
    player_color_t col;
    memset( &col, 0, sizeof(col));

    for( r=1.0; r>0; r-=0.1 )
      {
	col.blue = (int)(r * 255.0);
	col.red  = (int)(255.0 - r * 255.0);
	
	player_point_2d_t pts[4];
	pts[0].px = -r;
	pts[0].py = -r;
	pts[1].px = r;
	pts[1].py = -r;
	pts[2].px = r;
	pts[2].py = r;\
	pts[3].px = -r;
	pts[3].py = r;
	
	gp.DrawPolygon( pts, 4, 1, col);
	
	usleep(100000);
  }

  sleep(2);

  gp.Clear();

  return 0;
}
