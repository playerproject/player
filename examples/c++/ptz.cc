/*
 * ptz.cc
 *
 * a simple demo to show how to send commands to and get feedback from
 * the Sony PTZ camera.  this program will pan the camera in a loop
 * from side to side
 */

#include <playerclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char host[256] = "localhost";
int port = PLAYER_PORTNUM;

/* parse command-line args */
void
parse_args(int argc, char** argv)
{
  int i;

  i=1;
  while(i<argc)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i<argc)
        strcpy(host,argv[i]);
      else
      {
        //puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        port = atoi(argv[i]);
      else
      {
        //puts(USAGE);
        exit(1);
      }
    }
    else
    {
      //puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{ 
  parse_args(argc,argv);

  /* Connect to Player server */
  PlayerClient robot(host,port);

  /* Request sensor data */
  PtzProxy zp(&robot,0,'a');
  // connect to vision so that Stage will visualize the FOV
  //VisionProxy* vp = new VisionProxy(robot,0,'r');

  int dir = 1;
  double newpan;
  for(;;)
  {
    if(robot.Read())
      exit(1);

    zp.Print();

    if(zp.pan > DTOR(80) || zp.pan < DTOR(-80))
    {
      newpan = DTOR(dir*70);
      zp.SetCam(newpan,zp.tilt,zp.zoom);
      for(int i=0;i<10;i++)
      {
        if(robot.Read())
          exit(1);
      }
      zp.Print();
      dir = -dir;
    }
    newpan = zp.pan + dir * DTOR(5);
    zp.SetCam(newpan,zp.tilt,zp.zoom);
  }

  return(0);
}

