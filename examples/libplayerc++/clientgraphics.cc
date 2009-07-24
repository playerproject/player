/*
Copyright (c) 2005, Brad Kratochvil, Toby Collett, Brian Gerkey, Andrew Howard, ...
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the Player Project nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <libplayerc++/playerc++.h>
#include <iostream>
#include <cstring>

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

    player_point_2d_t pts[RAYS];

    double r;
    for( r=0; r<1.0; r+=0.05 )
      {
	int p;
	for( p=0; p<RAYS; p++ )
	  {
	    pts[p].px = r * cos(p * M_PI/(RAYS/2));
	    pts[p].py = r * sin(p * M_PI/(RAYS/2));
	  }

	gp.Color( 255,0,0,0 );
	gp.DrawPoints( pts, RAYS );

	usleep(500000);

	gp.Color( (int)(255.0*r),(int)(255-255.0*r),0,0 );
	gp.DrawPolyline( pts, RAYS/2 );

      }

    usleep(1000000);

    player_color_t col;
    memset( &col, 0, sizeof(col));

    for( r=1; r>0; r-=0.1 )
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

	usleep(300000);
  }

  usleep(1000000);

  gp.Clear();

  return 0;
}
