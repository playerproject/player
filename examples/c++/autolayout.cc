/*
 * autolayout.cc - use GPS teleport to auto layout robots in Stage
 */

#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playermulticlient.h>
#include <time.h>
#include <signal.h>

#define USAGE "USAGE: autolayout <host> <baseport> <num> <x0> <y0> <x1> <y1>"
               
// Convert radians to degrees
//
#define RTOD(r) ((r) * 180 / M_PI)

// Convert degrees to radians
//
#define DTOR(d) ((d) * M_PI / 180)

// Normalize angle to domain -pi, pi
//
#define NORMALIZE(z) atan2(sin(z), cos(z))

bool use_laser = false;
int numclients;
char host[256];
int baseport;
int x0,y0,x1,y1;

/* parse command-line args */
void
parse_args(int argc, char** argv)
{
  if(argc < 8)
  {
    puts(USAGE);
    exit(-1);
  }

  strncpy(host,argv[1],sizeof(host));
  baseport = atoi(argv[2]);
  numclients = atoi(argv[3]);
  x0 = (int)(atof(argv[4])*1000.0);
  y0 = (int)(atof(argv[5])*1000.0);
  x1 = (int)(atof(argv[6])*1000.0);
  y1 = (int)(atof(argv[7])*1000.0);
}

int main(int argc, char** argv)
{
  /* first, parse command line args */
  parse_args(argc,argv);

  PlayerClient* client;
  PositionProxy* pproxy;
  GpsProxy* gproxy;

  for(int i=0;i<numclients;i++)
  {
    printf("placing robot %d\n", i);

    client = new PlayerClient(host,baseport+i);
    pproxy = new PositionProxy(client,0,'r');
    gproxy = new GpsProxy(client,0,'r');

    client->SetFrequency(50);

    for(int i=0;i<5;i++)
    {
      if(client->Read())
        exit(-1);
    }

    while(pproxy->stalls)
    {
      int randx = x0 + (rand() % (x1-x0));
      int randy = y0 + (rand() % (y1-y0));
      int randh = rand() % 360;

      if(gproxy->Warp(randx,randy,randh))
        exit(-1);

      client->Read();
    }

    delete pproxy;
    delete gproxy;
    delete client;
  }

  return(0);
}
