/*
 * forage.cc - sonar obstacle avoidance with visual servo and gripping
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playerclient.h>
#include <math.h>
#include <values.h>

#define USAGE \
  "USAGE: forage [-h <host>] [-p <port>] [-c <channel>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -c <channel>: servo to this color <channel>"
 
#define NORMALIZE(z) atan2(sin(z), cos(z))
               
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
  // RANDOMWALK
  int randint;
  int randcount = 0;

  // COLLISION AVOID
  int avoidcount = 0;
  bool obs = false;
  unsigned short minfrontdistance = 450;

  // VISUAL SERVO / COLLECT
  unsigned int minarea = 1;
  unsigned int closearea = 4000;
  unsigned int sortofclosearea = 3000;

  // HOMING
  int home_x = 7000;
  int home_y = 7000;
  double home_size = 500; // mm

  // REVERSE HOMING
  int reverse_homing = 0;
  double last_bearing = 0;

  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  PlayerClient robot(host,port);

  // get device access
  PositionProxy pp(&robot,0,'a');
  SonarProxy sp(&robot,0,'r');
  VisionProxy vp(&robot,0,'r');
  GripperProxy gp(&robot,0,'a');
  GpsProxy gpsp(&robot,0,'r');

  int newturnrate,newspeed;
  //int lastdir = 1;
  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);
    //pp.Print();

    /* open the gripper */
    if(!gp.inner_break_beam)
      gp.SetGrip(GRIPopen,0);

    /* See if there is an obstacle in front */
    obs = (sp[2] < minfrontdistance ||
           sp[3] < minfrontdistance ||
           sp[4] < minfrontdistance ||
           sp[5] < minfrontdistance);

    if(obs || avoidcount || pp.stalls)
    {
      // OBSTACLE AVOIDANCE
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
    else if(gp.inner_break_beam)
    {
      //HOMING
      double dx = home_x-gpsp.xpos;
      double dy = home_y-gpsp.ypos;

      double dist = sqrt(dx*dx+dy*dy);

      if(dist < home_size)
      {
        // drop the puck
        newspeed = 0;
        gp.SetGrip(GRIPopen,0);
        reverse_homing = 100;
        last_bearing = MAXDOUBLE;
      }
      else
      {
        // go to home
        double bearing = RTOD(NORMALIZE(atan2(dy, dx)-DTOR(pp.compass-90)));
        if(fabs(bearing)>170.0)
          bearing=170.0;

        //puts("HOMING");
        //printf("dx: %f\tdy: %f\n", dx,dy);
        //printf("dist:%f bearing:%f\n", dist, bearing);
        newspeed = 200 - (int)(70000.0/dist);
        newturnrate = (int)(bearing/3.0);
      }
    }
    else if(reverse_homing)
    {
      // REVERSE HOMING
      double dx = home_x-gpsp.xpos;
      double dy = home_y-gpsp.ypos;
      double dist = sqrt(dx*dx+dy*dy);
      double bearing = RTOD(NORMALIZE(atan2(dy, dx)-DTOR(pp.compass-90)+M_PI));

      if(last_bearing < MAXDOUBLE && fabs(last_bearing-bearing) > 180)
        bearing=last_bearing;
      else
        last_bearing=bearing;

      newspeed = 200 - (int)(50000.0/dist);
      newturnrate = (int)(-bearing/3.0);

      reverse_homing--;

      //puts("REVERSE HOMING");
      //printf("dx: %f\tdy: %f\n", dx,dy);
      //printf("dist:%f bearing:%f\n", dist, bearing);
    }
    else if(vp.num_blobs[channel]>0)
    {
      // VISUAL SERVO
      //vp.Print();
      if(vp.blobs[channel][0].area < minarea)
        continue;

      int err = 80 - vp.blobs[channel][0].x;
      if(abs(err) > 0)
      {
        newturnrate = (int)(err / 3.0);
      }
      else
        newturnrate = 0;

      //gp.Print();
      // COLLECT
      if(vp.blobs[channel][0].area > closearea)
      {
        if(gp.paddles_open)
        {
          newspeed = 0;
          gp.SetGrip(GRIPclose,0);
        }
        else
        {
          newspeed = 100;
          gp.SetGrip(GRIPopen,0);
        }
      }
      else if(vp.blobs[channel][0].area > sortofclosearea)
        newspeed = 50;
      else
        newspeed = 200;
    }
    else
    {
      // RANDOM WALK
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
    
