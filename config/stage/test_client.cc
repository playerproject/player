#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
//#include <values.h>  // for MAXINT
#include <limits.h>  // for INT_MAX
#include <string.h> /* for strcmp() */
#include <unistd.h> /* for usleep() */
#include <sys/time.h>

#define USAGE \
  "USAGE: laserobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -i <index>: connect to devices with this index\n" \
  "       -m       : turn on motors (be CAREFUL!)"

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

// print a message with a timestamp
void print_tv( char* str, struct timeval* tv  )
{
  double sec = tv->tv_sec + (double)tv->tv_usec / 1e6;

  printf( "%s - %.3f\n", str, sec );
}

int main(int argc, char **argv)
{
  double minR, minL;

  parse_args(argc,argv);

  PlayerClient robot(host,port);
  PositionProxy pp(&robot,device_index,'a');
  LaserProxy lp(&robot,device_index,'r');

  printf("%s\n",robot.conn.banner);

  if(lp.access != 'r')
    exit(-1);
  
  for(int c=0; c<10; c++ )
    robot.Read();
    
  lp.GetConfigure();

  printf( "scan count %d\n",
	  lp.scan_count );

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
    
    print_tv( "client", &robot.timestamp );
    print_tv( "laser.generated", &lp.timestamp );
    print_tv( "laser.sent", &lp.senttime );
    //print_tv( "laser.received", &lp.receivedtime );

    int small = INT_MAX;
    for( int l=0; l<lp.scan_count; l++ )
      if( small>lp[l] ) small=lp[l];

    printf( "laser min %d\n", small );

    /* print current sensor data to console */
    //lp.Print();
    //pp.Print();

    /*
     * laser avoid (stolen from esben's java example)
     */

    minL=99999.9; 
    minR=99999.9;

    int scan_left_start = 0;
    int scan_left_end = lp.scan_count / 2;
    int scan_right_start = lp.scan_count / 2;
    int scan_right_end = lp.scan_count;

    for (int j=scan_left_start; j<scan_left_end; j++) {
      //printf("laser(%d):%d\n", j,robot.laser.ranges[j] & 0x1FFF);
      if (minR>lp[j])
        minR=lp[j];
    }
    for (int j=scan_right_start; j<scan_right_end; j++) {
      if (minL>lp[j])
        minL=lp[j];
    }
 
   printf("minR:%f\tminL:%f\n", minR,minL);

    int l=(1e5*minR)/500-100;
    int r=(1e5*minL)/500-100;
    if (l>100) 
      l=100; 
    if (r>100) 
      r=100;
    newspeed = r+l;
    newturnrate = r-l;
    newturnrate = min(newturnrate,40);
    newturnrate = max(newturnrate,-40);
    
    //convert from mm, degrees to m,radians
    
    double v = newspeed / 1000.0;
    double w = DTOR(newturnrate);

    printf( "v:%f  w:%f\n", v, w );
              
    /* write commands to robot */
    pp.SetSpeed(v,w);
  }
}
