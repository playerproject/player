#include <stdlib.h>
#include "nav200.h"

#define MIN 100
#define MAX 1000

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

  while(1)
  {
    if(testing.EnterPositioning())
    {
        printf("\n\n\nEntered positioning mode\n\n");
        if(testing.SetActionRadii(MIN,MAX))
          printf("changed operation radii\n");
        if(testing.GetPositionAuto(laser))
          printf("Position of the laser scanner: X=%d, Y=%d, orientation=%d, quality=%d, number of reflectors = %d\n",laser.pos.x,laser.pos.y,laser.orientation,laser.quality,laser.number);
    }
    else
    {
      fprintf(stderr,"unable to enter positioning mode\n");
      return EXIT_FAILURE;
    }
  }

  return 0;
}
