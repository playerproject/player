/*
 * randomwalk.cc - sonar obstacle avoidance with random walk
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playerclient.h>

#define USAGE \
  "USAGE: randomwalk [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"
               

CRobot robot;
bool turnOnMotors = false;

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

int main(int argc, char** argv)
{
  int randint;
  int randcount = 0;
  int avoidcount = 0;
  unsigned short minfrontdistance = 350;

  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  if(robot.Connect())
    exit(1);

  /* request read access on the sonars and write access to the wheels */
  if(robot.Request("pasr"))
    exit(1);

  /* maybe turn on the motors */
  if(turnOnMotors && robot.ChangeMotorState(1))
    exit(1);

  //if(robot.SetFrequency(1000))
    //exit(1);
  
  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    /* print current sensor data to console */
    //robot.Print();


    if(robot.sonar[2] < minfrontdistance ||
       robot.sonar[3] < minfrontdistance ||
       robot.sonar[4] < minfrontdistance ||
       robot.sonar[5] < minfrontdistance ||
       avoidcount || robot.position->stalls)
    {
      robot.newspeed = -150;

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(robot.sonar[1]+robot.sonar[15] < robot.sonar[7]+robot.sonar[8])
          robot.newturnrate = -40;
        else
          robot.newturnrate = 40;
      }
      avoidcount--;
    }
    else
    {
      avoidcount = 0;
      robot.newspeed = 200;

      /* update turnrate every 3 seconds */
      if(!randcount)
      {
        /* make random int tween -20 and 20 */
        randint = (1+(int)(40.0*rand()/(RAND_MAX+1.0))) - 20; 

        robot.newturnrate = randint;
        randcount = 20;
      }
      randcount--;
    }

    /* write commands to robot */
    robot.Write();
  }

  /* it's not necessary, but we could close the devices like this: */
  if(robot.Request("scpc"))
    exit(1);

  return(0);
}
    
