#include <math.h>
#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <limits.h>  // for INT_MAX
#include <string.h> /* for strcmp() */
#include <unistd.h> /* for usleep() */

#define USAGE \
  "USAGE: laserobstacleavoid [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;

int cmd_line_gotoxy = 0;
double gotox, gotoy, gotot;

/* easy little command line argument parser */
void parse_args(int argc, char** argv) {
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
    else if(!strcmp(argv[i],"-g"))
    {
      cmd_line_gotoxy = 1;
      if(++i<argc) {
        gotox = atof(argv[i]);
        if (++i<argc) {
          gotoy = atof(argv[i]);
          if (++i<argc) {
            gotot = atof(argv[i]);
          }
        } else {
          puts(USAGE);
          exit(1);
        }
      } else {
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

int main(int argc, char **argv) {
  parse_args(argc,argv);

  PlayerClient robot(host,port);
  LaserProxy lp(&robot,0,'r');
  // VFH should always be on position device index 1
  // A normal position device should always be on position index 0
  PositionProxy vfh_pp(&robot,1,'a');

  printf("%s\n",robot.conn.banner);

  if(lp.access != 'r')
    exit(-1);

  if (vfh_pp.access != 'a') {
    printf("No Access to VFH!\n");
    exit(-1);
  }
  
  /* maybe turn on the motors */
  if(turnOnMotors && vfh_pp.SetMotorState(1))
    exit(1);

  // as in a normal position device, vfh_pp.xpos, vfh_pp.ypos, vfh_pp.theta
  // are the robots x, y, and theta odometry data

  // vfh_pp.GoTo(x, y, theta) in meters and radians
  // goto x, y, theta -- theta is ignored for now
  // (x,y,theta) is in the robots global odometric
  // coordinate system -- i.e., (0,0,0) is the location when odometry
  // was last reset 
  // at the origin, forward = +x, left = +y
  // the robot will stop when it gets within ~50cm of the
  // goal.  client should monitor odometry to know when
  // robot is at goal


  // Placing a GoTo command here will tell the robot where to go 
  // and then can sit back and let it go...
//  vfh_pp.GoTo(100000, 200000, 0);
//  vfh_pp.GoTo(0, -3000, 0);

  // A goto command given on the command line
  if (cmd_line_gotoxy)
    vfh_pp.GoTo(gotox, gotoy, gotot);

  for(;;)
  {
    if(robot.Read())
      exit(1);

    if (!cmd_line_gotoxy) {
      /*
      // right wall follow
      vfh_pp.GoTo((int)rint(vfh_pp.xpos + sin(vfh_pp.theta) * 200.0), 
                  (int)rint(vfh_pp.ypos - cos(vfh_pp.theta) * 200.0), 
                  0);

      // left wall follow
      vfh_pp.GoTo((int)rint(vfh_pp.xpos - sin(vfh_pp.theta) * 200.0), 
                  (int)rint(vfh_pp.ypos + cos(vfh_pp.theta) * 200.0), 
                  0);
                  */

      // always try to go straight
      vfh_pp.GoTo((int)rint(vfh_pp.xpos + cos(vfh_pp.theta) * 200.0), 
                  (int)rint(vfh_pp.ypos + sin(vfh_pp.theta) * 200.0), 
                  0);
    }
  }
}

