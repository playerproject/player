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

int main(int argc, char **argv)
{
  double min_front_dist = 0.3;
  double really_min_front_dist = 0.15;
  char avoid;
  double minR, minL;

  parse_args(argc,argv);

  PlayerClient robot(host,port);
  PositionProxy pp(&robot,device_index,'a');
  SonarProxy sp(&robot,device_index,'r');
  EnergyProxy ep(&robot,device_index,'r');
  
  printf("%s\n",robot.conn.banner);

  if(pp.access != 'a')
    {
      puts( "failed to access position device" );
      exit(-1);
    }

  if(sp.access != 'r')
    {
      puts( "failed to access sonar device" );
      exit(-1);
    }
  
  for(int c=0; c<5; c++ )
    robot.Read();
  
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
    sp.Print();
    pp.Print();

    /*
     * sonar avoid.
     *   policy (pretty stupid):
     *     if(object really close in front)
     *       backup and turn away;
     *     else if(object close in front)
     *       stop and turn away;
     */
    avoid = 0;
    newspeed = 0.400;

    if (avoid == 0)
    {
        if((sp[0] < really_min_front_dist) ||
           (sp[1] < really_min_front_dist) ||
           //(sp[4] < really_min_front_dist) ||
           (sp[7] < really_min_front_dist))
        {
            avoid = 50;
            newspeed = -0.100;
        }
        else if((sp[0] < min_front_dist) ||
                (sp[1] < min_front_dist) ||
                //(sp[4] < min_front_dist) ||
                (sp[7] < min_front_dist))
        {
            newspeed = 0;
            avoid = 50;
        }
    }

    if(avoid > 0)
    {  
      if((sp[1] + sp[2]) < 
         (sp[6] + sp[7]))
        newturnrate = DTOR(-30);
      else
        newturnrate = DTOR(30);
      avoid--;
    }
    else
      newturnrate = 0;

    /* write commands to robot */
    pp.SetSpeed(newspeed,newturnrate);
  }
}
