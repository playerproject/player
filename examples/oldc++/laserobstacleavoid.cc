#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <oldplayerclient.h>  // for player client stuff
#include <values.h>  // for MAXINT
#include <string.h> /* for strcmp() */

#define USAGE \
  "USAGE: laserobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

// our robot
PlayerClient robot;
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
  int minR, minL;

  parse_args(argc,argv);

  // if the robot.* methods return non-zero, then there was an error...
  if(robot.Connect())
    exit(1);

  if(robot.RequestDeviceAccess(PLAYER_LASER_CODE, PLAYER_READ_MODE))
    exit(1);
  if(robot.RequestDeviceAccess(PLAYER_POSITION_CODE, PLAYER_WRITE_MODE))
    exit(1);

  /* maybe turn on the motors */
  if(turnOnMotors && robot.ChangeMotorState(1))
    exit(1);

  //robot.SetLaserConfig(50,-9000,9000,false);

  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);


    /* print current sensor data to console */
    //robot.Print();

    /*
     * laser avoid (stolen from esben's java example)
     */
    // Do obstackle avoidance for 400 time steps
    minL=MAXINT; 
    minR=MAXINT;
    for (int j=0; j<180; j++) {
      //printf("laser(%d):%d\n", j,robot.laser->ranges[j] & 0x1FFF);
      if (minR>(robot.laser->ranges[j] & 0x1FFF))
        minR=(robot.laser->ranges[j] & 0x1FFF);
    }
    for (int j=181; j<361; j++) {
      if (minL>(robot.laser->ranges[j] & 0x1FFF))
        minL=(robot.laser->ranges[j] & 0x1FFF);
    }
    //printf("minR:%d\tminL:%d\n", minR,minL);
    int l=(100*minR)/500-100;
    int r=(100*minL)/500-100;
    if (l>150) 
      l=150; 
    if (r>150) 
      r=150;
    *(robot.newspeed) = r+l;
    *(robot.newturnrate) = r-l;
    //printf("speed: %d\tturn:%d\n", *(robot.newspeed), *(robot.newturnrate));
                  
    /* write commands to robot */
    robot.Write();
  }
}
