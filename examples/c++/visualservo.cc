/*
 * visualservo.cc - sonar obstacle avoidance with visual servo
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playerclient.h>

#define USAGE \
  "USAGE: visualservo [-h <host>] [-p <port>] [-c <channel>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -c <channel>: servo to this color <channel>\n" \
  "       -l       : use laser instead of sonar\n" \
  "       -m       : turn on motors (be CAREFUL!)"
               

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;
int channel = 0;

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
    else if(!strcmp(argv[i],"-c"))
    {
      if(++i<argc)
        channel = atoi(argv[i]);
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
  bool obs = false;
  unsigned short minfrontdistance = 450;
  unsigned int minarea = 50;

  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  PlayerClient robot(host,port);
  SonarProxy sp(&robot,0,'r');
  VisionProxy vp(&robot,0,'r');

  /* request read access on the sonars and all access to the wheels */
  P2PositionProxy p2pp(&robot,0,'a');
  PositionProxy &pp = p2pp;

  /* maybe turn on the motors */
  if(turnOnMotors && pp.SetMotorState(1))
    exit(1);

  int newturnrate=0,newspeed=0;
  //int lastdir = 1;
  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    vp.Print();

    /* See if there is an obstacle in front */
    obs = (sp[2] < minfrontdistance ||
           sp[3] < minfrontdistance ||
           sp[4] < minfrontdistance ||
           sp[5] < minfrontdistance);

    if(obs || avoidcount || pp.Stalls())
    {
      newspeed = 0; //-150;

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(sp[1]+sp[15] < sp[7]+sp[8])
          newturnrate = -40;
        else
          newturnrate = 40;
      }
      avoidcount--;
    }
    else if(vp.num_blobs[channel]>0)
    {
      if(vp.blobs[channel][0].area < minarea)
        continue;

      int err = 80 - vp.blobs[channel][0].x;
      if(abs(err) > 5)
      {
        newturnrate = (int)(err / 3.0);
      }
      else
        newturnrate = 0;
      
      //newspeed = 0;
      newspeed = 200;
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
    
