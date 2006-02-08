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

  // we throw exceptions on creation if we fail
  try
  {
    using namespace PlayerCc;

    PlayerClient robot(gHostname, gPort);
    Position2dProxy pp(&robot, gIndex);
    LaserProxy lp(&robot, gIndex);
    
    std::cout << robot << std::endl;

    // go into read-think-act loop
    for(;;)
    {
      double newspeed = 0;
      double newturnrate = 0;
      double minR = 1e9;
      double minL = 1e9;

      // this blocks until new data comes; 10Hz by default
      robot.Read();

      // laser avoid (stolen from esben's java example)
      uint count = lp.GetCount();
      for (uint j=0; j < count/2; ++j)
      {
        if (minR > lp[j])
          minR = lp[j];
      }
      for (uint j = count/2; j < count; ++j)
      {
        if (minL > lp[j])
          minL = lp[j];
      }
      std::cout << "minR: " << minR
                << "minL: " << minL
                << std::endl;

      double l = (1e5*minR)/500-100;
      double r = (1e5*minL)/500-100;

      if (l > 100)
        l = 100;
      if (r > 100)
        r = 100;

      newspeed = (r+l)/1e3;

      newturnrate = (r-l);
      newturnrate = limit(newturnrate, -40.0, 40.0);
      newturnrate = dtor(newturnrate);

      std::cout << "speed: " << newspeed
                << "turn: " << newturnrate
                << std::endl;

      // write commands to robot
      pp.SetSpeed(newspeed, newturnrate);      
    }
  }
  catch (PlayerCc::PlayerError e)
  {
    std::cerr << e << std::endl;
    return -1;
  }
}
