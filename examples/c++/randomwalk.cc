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
  "       -l       : use laser instead of sonar\n" \
  "       -m       : turn on motors (be CAREFUL!)"
               

bool turnOnMotors = false;
bool use_laser = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;

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
  PlayerClient robot(host,port);
  LaserProxy lp(&robot,0);
  SonarProxy sp(&robot,0);

  /* request read access on the sonars and all access to the wheels */
  PositionProxy pp(&robot,0,'a');

  if (use_laser)
    lp.ChangeAccess('r');
  else
    sp.ChangeAccess('r');

  /* maybe turn on the motors */
  if(turnOnMotors && pp.SetMotorState(1))
    exit(1);

  int newturnrate,newspeed;
  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    /* See if there is an obstacle in front */
    if (use_laser)
    {
        obs = false;
        for (int i = 0; i < lp.range_count; i++)
        {
            if ((lp.ranges[i] & 0x1FFF) < minfrontdistance)
                obs = true;
        }
    }
    else
    {
        obs = (sp.ranges[2] < minfrontdistance ||
               sp.ranges[3] < minfrontdistance ||
               sp.ranges[4] < minfrontdistance ||
               sp.ranges[5] < minfrontdistance);
    }

    if(obs || avoidcount || pp.stalls)
    {
      newspeed = 0; //-150;

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(use_laser)
        {
          if(lp.min_left < lp.min_right)
            newturnrate = -40;
          else
            newturnrate = 40;
        }
        else
        {
          if(sp.ranges[1]+sp.ranges[15] < sp.ranges[7]+sp.ranges[8])
            newturnrate = -40;
          else
            newturnrate = 40;
        }
      }
      avoidcount--;
    }
    else
    {
      avoidcount = 0;
      newspeed = 200;

      /* update turnrate every 3 seconds */
      if(!randcount)
      {
        /* make random int tween -20 and 20 */
        //randint = (1+(int)(40.0*rand()/(RAND_MAX+1.0))) - 20;
        randint = rand() % 41 - 20;

        newturnrate = randint;
        randcount = 20;
      }
      randcount--;
    }

    /* write commands to robot */
    pp.SetSpeed(newspeed,newturnrate);
  }

  return(0);
}
    
