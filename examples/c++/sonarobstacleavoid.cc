#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */

#define USAGE \
  "USAGE: sonarobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
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
  unsigned short min_front_dist = 500;
  unsigned short really_min_front_dist = 300;
  char avoid;

  parse_args(argc,argv);

  PlayerClient robot(host,port);

  PositionProxy pp(&robot,0,'a');
  //LaserProxy lp(&robot,0,'r');
  SonarProxy sp(&robot,0,'r');

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
    //sp.Print();

    /*
     * sonar avoid.
     *   policy (pretty stupid):
     *     if(object really close in front)
     *       backup and turn away;
     *     else if(object close in front)
     *       stop and turn away;
     */
    avoid = 0;
    newspeed = 200;

    if (avoid == 0)
    {
        if((sp[2] < really_min_front_dist) ||
           (sp[3] < really_min_front_dist) ||
           (sp[4] < really_min_front_dist) ||
           (sp[5] < really_min_front_dist))
        {
            avoid = 50;
            newspeed = -100;
        }
        else if((sp[2] < min_front_dist) ||
                (sp[3] < min_front_dist) ||
                (sp[4] < min_front_dist) ||
                (sp[5] < min_front_dist))
        {
            newspeed = 0;
            avoid = 50;
        }
    }

    if(avoid > 0)
    {  
      if((sp[0] + sp[1]) < 
         (sp[6] + sp[7]))
        newturnrate = 30;
      else
        newturnrate = -30;
      avoid--;
    }
    else
      newturnrate = 0;

    /* write commands to robot */
    pp.SetSpeed(2*newspeed,newturnrate);
  }
}
