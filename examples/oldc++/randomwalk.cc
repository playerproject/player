/*
 * randomwalk.cc - sonar obstacle avoidance with random walk
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <oldplayerclient.h>

#define USAGE \
  "USAGE: randomwalk [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -l       : use laser instead of sonar\n" \
  "       -m       : turn on motors (be CAREFUL!)"
               

PlayerClient robot;
bool turnOnMotors = false;
bool use_laser = false;

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
    else if(!strcmp(argv[i],"-l"))
    {
      use_laser = true;
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
  bool obs = false;
  unsigned short minfrontdistance = 450;

  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  if(robot.Connect())
    exit(1);

  /* request read access on the sonars and all access to the wheels */
  if(robot.RequestDeviceAccess(PLAYER_POSITION_CODE,PLAYER_ALL_MODE))
    exit(1);
  if (use_laser)
    if(robot.RequestDeviceAccess(PLAYER_LASER_CODE,PLAYER_READ_MODE))
      exit(1);
  else
    if(robot.RequestDeviceAccess(PLAYER_SONAR_CODE,PLAYER_READ_MODE))
      exit(1);

  /* maybe turn on the motors */
  //if(turnOnMotors && robot.ChangeMotorState(1))
    //exit(1);
  if(robot.ChangeSonarState(0))
    exit(1);

  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    /* print current sensor data to console */
    /*robot.Print();*/

    /* See if there is an obstacle in front */
    if (use_laser)
    {
        obs = false;
        for (int i = 0; i < robot.laser->range_count; i++)
        {
            if ((robot.laser->ranges[i] & 0x1FFF) < minfrontdistance)
                obs = true;
        }
    }
    else
    {
        obs = (robot.sonar[2] < minfrontdistance ||
               robot.sonar[3] < minfrontdistance ||
               robot.sonar[4] < minfrontdistance ||
               robot.sonar[5] < minfrontdistance);
    }

    if(obs || avoidcount || robot.position->stalls)
    {
      *robot.newspeed = 0; //-150;

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(robot.sonar[1]+robot.sonar[15] < robot.sonar[7]+robot.sonar[8])
          *robot.newturnrate = -40;
        else
          *robot.newturnrate = 40;
      }
      avoidcount--;
    }
    else
    {
      avoidcount = 0;
      *robot.newspeed = 200;

      /* update turnrate every 3 seconds */
      if(!randcount)
      {
        /* make random int tween -20 and 20 */
        //randint = (1+(int)(40.0*rand()/(RAND_MAX+1.0))) - 20;
        randint = rand() % 41 - 20;

        *robot.newturnrate = randint;
        randcount = 20;
      }
      randcount--;
    }

    /* write commands to robot */
    robot.Write();
  }

  return(0);
}
    
