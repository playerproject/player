#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <values.h>  // for MAXINT
#include <string.h> /* for strcmp() */

#define USAGE \
  "USAGE: laserobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;

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
        strcpy(host,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        port = atoi(argv[i]);
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

  PlayerClient robot(host,port);
  PositionProxy pp(&robot,0,'w');
  LaserProxy lp(&robot,0,'r');
  
  /* maybe turn on the motors */
  if(turnOnMotors && pp.SetMotorState(1))
    exit(1);

  int newspeed, newturnrate;
  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    /* print current sensor data to console */
    //lp.Print();
    //pp.Print();

    /*
     * laser avoid (stolen from esben's java example)
     */
    // Do obstackle avoidance for 400 time steps
    minL=MAXINT; 
    minR=MAXINT;
    for (int j=0; j<180; j++) {
      //printf("laser(%d):%d\n", j,robot.laser.ranges[j] & 0x1FFF);
      if (minR>lp.ranges[j])
        minR=lp.ranges[j];
    }
    for (int j=181; j<361; j++) {
      if (minL>lp.ranges[j])
        minL=lp.ranges[j];
    }
    //printf("minR:%d\tminL:%d\n", minR,minL);
    int l=(100*minR)/500-100;
    int r=(100*minL)/500-100;
    if (l>100) 
      l=100; 
    if (r>100) 
      r=100;
    newspeed = r+l;
    newturnrate = r-l;
    newturnrate = max(newturnrate,40);
    newturnrate = min(newturnrate,-40);
    //printf("speed: %d\tturn:%d\n", *(robot.newspeed), *(robot.newturnrate));
                  
    /* write commands to robot */
    pp.SetSpeed(newspeed,newturnrate);
  }
}
