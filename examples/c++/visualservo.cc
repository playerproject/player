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
  double minfrontdistance = 0.450;
  unsigned int minarea = 50;

  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  PlayerClient robot(host,port);
  SonarProxy sp(&robot,0,'r');
  BlobfinderProxy vp(&robot,0,'r');

  /* request read access on the sonars and all access to the wheels */
  PositionProxy pp(&robot,0,'a');

  /* maybe turn on the motors */
  if(turnOnMotors && pp.SetMotorState(1))
    exit(1);

//  vp.SetTrackingColor(47,100,95,118,20,66);
  double newturnrate=0,newspeed=0;
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

    if(obs || avoidcount || pp.stall)
    {
      newspeed = 0; //-150;

      /* once we start avoiding, continue avoiding for 2 seconds */
      /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
      if(!avoidcount)
      {
        avoidcount = 15;
        randcount = 0;

        if(sp[1]+sp[15] < sp[7]+sp[8])
          newturnrate = DTOR(-40);
        else
          newturnrate = DTOR(40);
      }
      avoidcount--;
    }
    else if(vp.blob_count>0)
    {
      if(vp.blobs[0].id != channel)
        continue;
      if(vp.blobs[0].area < minarea)
        continue;

      int err = 80 - vp.blobs[0].x;
      if(abs(err) > 5)
      {
        newturnrate = DTOR(err / 3.0);
      }
      else
        newturnrate = 0;
      
      //newspeed = 0;
      newspeed = 0.200;
    }
    else
    {
      avoidcount = 0;
      newspeed = 0.200;

      /* update turnrate every 3 seconds */
      if(!randcount)
      {
        /* make random int tween -20 and 20 */
        //randint = (1+(int)(40.0*rand()/(RAND_MAX+1.0))) - 20;
        randint = rand() % 41 - 20;

        newturnrate = DTOR(randint);
        randcount = 20;
      }
      randcount--;
    }

    /* write commands to robot */
    pp.SetSpeed(newspeed,newturnrate);
  }

  return(0);
}
    
