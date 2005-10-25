/*
 * ptz.cc
 *
 * a simple demo to show how to send commands to and get feedback from
 * the Sony PTZ camera.  this program will pan the camera in a loop
 * from side to side
 *
 * @todo: this has been ported to libplayerc++, but not tested
 */

#include <libplayerc++/playerc++.h>

#include <iostream>
#include <string>

// defines gHostname, gPort, ...
#include "config.h"

int main(int argc, char **argv)
{
  parse_args(argc,argv);

  using namespace PlayerCc;

  /* Connect to Player server */
  PlayerClient robot(gHostname, gPort);

  /* Request sensor data */
  PtzProxy zp(&robot, gIndex);

  int dir = 1;
  double newpan;
  for(;;)
  {
    try
    {
      robot.Read();

      std::cout << zp << std::endl;

      if(zp.GetPan() > DTOR(80) || zp.GetPan() < DTOR(-80))
      {
        newpan = DTOR(dir*70);
        zp.SetCam(newpan,zp.GetTilt(),zp.GetZoom());
        for(int i=0;i<10;i++)
        {
          robot.Read();
        }
        std::cout << zp << std::endl;
        dir = -dir;
      }
      newpan = zp.GetPan() + dir * DTOR(5);
      zp.SetCam(newpan,zp.GetTilt(),zp.GetZoom());
    }
    catch (PlayerError e)
    {
      std::cerr << e << std::endl;
      return 1;
    }
  }

  return(0);
}

