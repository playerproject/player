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

/*
 * ptz.cc
 *
 * a simple demo to show how to send commands to and get feedback from
 * the Sony PTZ camera.  this program will pan the camera in a loop
 * from side to side
 */

#include <libplayerc++/playerc++.h>

#include <iostream>
#include <string>

// defines gHostname, gPort, ...
#include "args.h"

int main(int argc, char **argv)
{
  parse_args(argc,argv);

  using namespace PlayerCc;

  /* Connect to Player server */
  PlayerClient robot(gHostname, gPort);

  /* Request sensor data */
  PtzProxy zp(&robot, gIndex);
  CameraProxy cp(&robot, gIndex);

  int dir = 1;
  double newpan;
  for(;;)
  {
    try
    {
      robot.Read();

      std::cout << zp << std::endl;

      if(zp.GetPan() > dtor(40) || zp.GetPan() < dtor(-40))
      {
        newpan = dtor(dir*30);
        zp.SetCam(newpan, zp.GetTilt(), zp.GetZoom());
        for(int i=0; i<10; ++i)
        {
          robot.Read();
        }
        std::cout << zp << std::endl;
        dir = -dir;
      }
      newpan = zp.GetPan() + dir * dtor(5);
      zp.SetCam(newpan, zp.GetTilt(), zp.GetZoom());
    }
    catch (PlayerError & e)
    {
      std::cerr << e << std::endl;
      return 1;
    }
  }

  return(0);
}

