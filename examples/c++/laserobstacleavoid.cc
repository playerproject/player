#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
//#include <values.h>  // for MAXINT
#include <limits.h>  // for INT_MAX
#include <string.h> /* for strcmp() */
#include <unistd.h> /* for usleep() */

#define USAGE \
  "USAGE: laserobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

#ifndef MIN
  #define MIN(a,b) ((a < b) ? (a) : (b))
#endif
#ifndef MAX
  #define MAX(a,b) ((a > b) ? (a) : (b))
#endif

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;
int device_index = 0; // use this to access the nth indexed position and laser devices

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
    else if(!strcmp(argv[i],"-i"))
    {
      if(++i<argc)
        device_index = atoi(argv[i]);
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
  double minR, minL;

  parse_args(argc,argv);

  PlayerClient robot(host,port);
  PositionProxy pp(&robot,device_index,'w');
  LaserProxy lp(&robot,device_index,'r');

  printf("%s\n",robot.conn.banner);

  if(lp.access != 'r')
    {
      puts( "can't read from laser" );
      exit(-1);
    }  
 
  /* maybe turn on the motors */
  //if(turnOnMotors && pp.SetMotorState(1))
  //exit(1);

  double newspeed, newturnrate;
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

    minL=1e9;
    minR=1e9;
    for (int j=0; j<lp.scan_count/2; j++) {
      if (minR>lp[j])
        minR=lp[j];
    }
    for (int j=lp.scan_count/2; j<lp.scan_count; j++) {
      if (minL>lp[j])
        minL=lp[j];
    }
    printf("minR:%.3f\tminL:%.3f\n", minR,minL);
    double l=(1e5*minR)/500-100;
    double r=(1e5*minL)/500-100;
    if (l>100) 
      l=100; 
    if (r>100) 
      r=100;

    newspeed = (r+l)/1e3;

    newturnrate = (r-l);
    newturnrate = MIN(newturnrate,40);
    newturnrate = MAX(newturnrate,-40);
    newturnrate = DTOR(newturnrate);

    printf( "speed %f  turn %f\n", newspeed, newturnrate );
    
    /* write commands to robot */
    pp.SetSpeed( newspeed, newturnrate );
  }
}
