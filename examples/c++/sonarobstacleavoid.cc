#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */

#define USAGE \
  "USAGE: sonarobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

// our robot
CRobot robot;
bool turnOnMotors = false;

/* easy little command line argument parser */
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
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        robot.port = atoi(argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-m"))
    {
      turnOnMotors = true;
    }
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{
  unsigned short min_front_dist = 500;
  unsigned short really_min_front_dist = 300;
  char avoid;

  parse_args(argc,argv);

  // if the robot.* methods return non-zero, then there was an error...
  if(robot.Connect())
    exit(1);

  if(robot.Request("srpw"))
    exit(1);

  /* maybe turn on the motors */
  if(turnOnMotors && robot.ChangeMotorState(1))
    exit(1);

  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    /* print current sensor data to console */
    //robot.Print();

    /*
     * sonar avoid.
     *   policy (pretty stupid):
     *     if(object really close in front)
     *       backup and turn away;
     *     else if(object close in front)
     *       stop and turn away;
     */
    avoid = 0;
    robot.newspeed = 200;

    if((robot.sonar[2] < really_min_front_dist) ||
       (robot.sonar[3] < really_min_front_dist) ||
       (robot.sonar[4] < really_min_front_dist) ||
       (robot.sonar[5] < really_min_front_dist))
    {
      avoid = 1;
      robot.newspeed = -100;
    }
    else if((robot.sonar[2] < min_front_dist) ||
            (robot.sonar[3] < min_front_dist) ||
            (robot.sonar[4] < min_front_dist) ||
            (robot.sonar[5] < min_front_dist))
    {
      robot.newspeed = 0;
      avoid = 1;
    }

    if(avoid)
    {
      if((robot.sonar[0] + robot.sonar[1]) < 
         (robot.sonar[6] + robot.sonar[7]))
        robot.newturnrate = 30;
      else
        robot.newturnrate = -30;
    }
    else
      robot.newturnrate = 0;

    /* write commands to robot */
    robot.Write();
  }
}
