#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
//#include <values.h>  // for MAXINT
#include <limits.h>  // for INT_MAX
#include <string.h> /* for strcmp() */
#include <unistd.h> /* for usleep() */

#define USAGE \
  "USAGE: shapetracker [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
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
  int minR, minL;

  parse_args(argc,argv);

  PlayerClient robot(host,port);
  //CameraProxy cp(&robot,0,'r');

  BlobfinderProxy bp(&robot, 0, 'r');
  HUDProxy hp(&robot, 0, 'w');

  int newspeed, newturnrate;
  int i,j;
  float red[3];
  float blue[3];

  red[0] = 255.0;
  red[1] = 0.0;
  red[2] = 0.0;

  blue[0] = 0.0;
  blue[1] = 0.0;
  blue[2] = 255.0;


  /* go into read-think-act loop */
  for(;;)
  {
    /* this blocks until new data comes; 10Hz by default */
    if(robot.Read())
      exit(1);

    //printf( "Num Blobs[%d]\n",bp.num_blobs[8]);

    for (i=6; i<=16; i+=2)
    {
      for (j=0; j < 5/*bp.num_blobs[i]*/; j++)
      {
        if (j < bp.blob_count)
        {
          hp.SetColor( red );
          hp.SetStyle(0);
          hp.DrawBox( i*10+j, bp.blobs[j].left,
              bp.blobs[j].bottom,
              bp.blobs[j].right, 
              bp.blobs[j].top);

          hp.SetColor(blue);
          hp.SetStyle(1);
          hp.DrawCircle( -i*10+j, bp.blobs[j].x, bp.blobs[j].y, 8 );

        } else {
          hp.Remove(i*10+j);
          hp.Remove(-i*10+j);
        }
      }
    }
  }
}
