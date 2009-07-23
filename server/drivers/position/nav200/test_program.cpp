/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006
 *     Kathy Fung, Toby Collett
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

/*#include <stdlib.h>
#include "nav200.h"

#define XMIN 100
#define XMAX 1000

int main(int argc,char ** argv)
{
  Nav200 testing;
  testing.Initialise();
  if (!testing.EnterStandby())
  {
      fprintf(stderr,"unable to enter standby mode\n");
      return EXIT_FAILURE;
  }
  LaserPos laser;

  if(testing.EnterPositioning())
  {
      printf("\n\n\nEntered positioning mode\n\n");
      if(testing.SetActionRadii(XMIN,XMAX))
	printf("changed operation radii\n");
  }
  else
  {
      fprintf(stderr,"unable to enter positioning mode\n");
      return EXIT_FAILURE;
  }
  
  while(1)
  {
      if(testing.GetPositionAuto(laser))
	printf("Position of the laser scanner: X=%d, Y=%d, orientation=%d, quality=%d, number of reflectors = %d\n",laser.pos.x,laser.pos.y,laser.orientation,laser.quality,laser.number);
  }

  return 0;
}
*/

int main()
{}
