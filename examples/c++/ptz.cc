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

PlayerClient robot;

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
        strcpy(robot.host,argv[i]);
      else
      {
        //puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        robot.port = atoi(argv[i]);
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
  if(robot.Connect())
    exit(1);

  /* Request sensor data */
  if(robot.RequestDeviceAccess(PLAYER_PTZ_CODE, PLAYER_ALL_MODE) == -1)
    exit(1);

  int dir = 1;
  for(;;)
  {
    if(robot.Read())
      exit(1);

    robot.Print();

    if(robot.ptz->pan > 80 || robot.ptz->pan < -80)
    {
      *robot.newpan = dir*70;
      robot.Write();
      for(int i=0;i<10;i++)
      {
        if(robot.Read())
          exit(1);
      }
      robot.Print();
      dir = -dir;
    }
    *robot.newpan = robot.ptz->pan + dir * 5;
    robot.Write();
  }

  return(0);
}

