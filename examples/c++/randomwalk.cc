/*
 * randomwalk.cc - sonar obstacle avoidance with random walk
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playerclient.h>
#include <time.h>

#define USAGE \
  "USAGE: randomwalk [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host> : connect to Player on this host\n" \
  "       -p <port> : connect to Player on this TCP port\n" \
  "       -l        : use laser instead of sonar\n" \
  "       -m        : turn on motors (be CAREFUL!)\n" \
  "       -s <speed>: move at this speed (integer)\n" \
  "       -a <speed>: move to avoid obstacles at this speed (integer)\n" \
  "       -t <rate> : turn rate when avoiding obstacle (integer)\n" \
  "       -d <dist> : minimum tolerated sonar distance (integer)\n"  
               

bool turnOnMotors = false;
bool use_laser = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;
char auth_key[PLAYER_KEYLEN];

unsigned short minfrontdistance = 350;
short speed = 200;
short avoidspeed = 0; // -150;
short turnrate = 40;


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
    else if(!strcmp(argv[i],"-k"))
    {
      if(++i<argc)
        strncpy(auth_key,argv[i],sizeof(auth_key));
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
    else if(!strcmp(argv[i], "-s"))
    {
      speed = atoi(argv[++i]);
    }
    else if(!strcmp(argv[i], "-a"))
    {
      avoidspeed = atoi(argv[++i]);
    }
    else if(!strcmp(argv[i], "-d"))
    {
      minfrontdistance = atoi(argv[++i]);
    }
    else if(!strcmp(argv[i], "-t"))
    { 
      minfrontdistance = atoi(argv[++i]);
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


  /* first, parse command line args */
  parse_args(argc,argv);

  // seed the RNG
  srand( (unsigned int)time(0) );

  /* Connect to the Player server */
  PlayerClient robot(host,port);

  if(strlen(auth_key))
  {
    if(robot.Authenticate(auth_key))
    {
      puts("Authentication failed.");
      exit(1);
    }
  }


  LaserProxy lp(&robot,0);
  SonarProxy sp(&robot,0);
  PositionProxy pp(&robot,0,'a');

  if(pp.GetAccess() == 'e') {
    puts("Error getting position device access!");
    exit(1);
  }

  if (use_laser) {
    lp.ChangeAccess('r');
    if(lp.GetAccess() == 'e') {
      puts("Error getting laser device access!");
      exit(1);
    }
  } else {
    sp.ChangeAccess('r');
    if(sp.GetAccess() == 'e') {
      puts("Error getting sonar device access!.");
      exit(1);
    }
  }

  /* maybe turn on the motors */
  if(turnOnMotors && pp.SetMotorState(1))
    exit(1);

  for(int j=0;j<10;j++)
  {
    if(robot.Read())
      exit(1);
  }

  int newturnrate=0,newspeed=0;
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
        //printf("comparing minfrontdiatance=%d to ranges 2=%d, 3=%d, 4=%d,  5=%d.\n", minfrontdistance, sp.ranges[2], sp.ranges[3], sp.ranges[4], sp.ranges[5]);
        obs = ( (sp.ranges[2] < minfrontdistance) ||   // 0?
                (sp.ranges[3] < minfrontdistance) ||   
                (sp.ranges[4] < minfrontdistance) ||   // 0?
                (sp.ranges[5] < minfrontdistance) );    
    }

    if(obs || avoidcount || pp.stall)
    {
      newspeed = avoidspeed; 

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(use_laser)
        {
          if(lp.min_left < lp.min_right)
            newturnrate = -turnrate;
          else
            newturnrate = turnrate;
        }
        else
        {
          if(sp.ranges[1]+sp.ranges[15] < sp.ranges[7]+sp.ranges[8])
            newturnrate = -turnrate;
          else
            newturnrate = turnrate;
        }
      }
      avoidcount--;
    }
    else
    {
      avoidcount = 0;
      newspeed = speed;

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
    
