/*
 * multirandom.cc - sonar obstacle avoidance with random walk for
 *                  multiple robots
 */

#include <math.h>
#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playermulticlient.h>
#include <time.h>
#include <signal.h>

#define USAGE "USAGE: multirandom <host> <baseport> <num>"
               
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

/* parse command-line args */
void
parse_args(int argc, char** argv)
{
  if(argc < 4)
  {
    puts(USAGE);
    exit(-1);
  }

  strncpy(host,argv[1],sizeof(host));
  baseport = atoi(argv[2]);
  numclients = atoi(argv[3]);
}

#define STRAIGHT 0
#define TURN 1

int main(int argc, char** argv)
{
  int randint;
  unsigned short minfrontdistance = 400;
  bool obs = false;
  bool write = false;

  /* first, parse command line args */
  parse_args(argc,argv);

  struct timeval curr;
  // seed the RNG
  gettimeofday(&curr,NULL);
  srand(curr.tv_usec);

  PlayerMultiClient mc;

  PlayerClient** clients = new PlayerClient*[numclients];
  PositionProxy** pproxies = new PositionProxy*[numclients];
  SonarProxy** sproxies = new SonarProxy*[numclients];

  int* randcounts = new int[numclients];
  int* avoidcounts = new int[numclients];
  int* modes = new int[numclients];

  memset(randcounts,0,sizeof(int)*numclients);
  memset(avoidcounts,0,sizeof(int)*numclients);
  memset(modes,0,sizeof(int)*numclients);

  for(int i=0;i<numclients;i++)
  {
    clients[i] = new PlayerClient(host,baseport+i);
    mc.AddClient(clients[i]);

    pproxies[i] = new PositionProxy(clients[i],0,'a');
    sproxies[i] = new SonarProxy(clients[i],0,'r');
  }

  int newturnrate=0,newspeed=0;

  if(mc.Read())
    exit(1);
  if(mc.Read())
    exit(1);
  if(mc.Read())
    exit(1);

  for(;;)
  {
    if(mc.Read())
      exit(1);

    // check each client to see if it got new data
    //puts("**************");
    for(int i=0;i<numclients;i++)
    {
      write = true;
      if(clients[i]->fresh)
      {
        obs = (sproxies[i]->ranges[1] < minfrontdistance ||
               sproxies[i]->ranges[2] < minfrontdistance ||
               sproxies[i]->ranges[3] < minfrontdistance ||
               sproxies[i]->ranges[4] < minfrontdistance ||
               sproxies[i]->ranges[5] < minfrontdistance ||
               sproxies[i]->ranges[6] < minfrontdistance);

        if(obs || avoidcounts[i] || pproxies[i]->stalls)
        {
          randcounts[i] = 0;
          modes[i]=STRAIGHT;
          if(!obs && pproxies[i]->stalls && !(avoidcounts[i]))
          {
            newturnrate = 0;
            randint = rand() % 2;
            if(randint)
              newspeed = -100;
            else
              newspeed = 100;
            avoidcounts[i] = 10;
          }
          else
          {
            /* once we start avoiding, continue avoiding for 2 seconds */
            /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
            if(!(avoidcounts[i]) || (obs && newspeed))
            {
              avoidcounts[i] = 15;

              newspeed = 0;
              if(sproxies[i]->ranges[1]+sproxies[i]->ranges[15] < 
                 sproxies[i]->ranges[7]+sproxies[i]->ranges[8])
                newturnrate = -20;
              else
                newturnrate = 20;
            }
            else
            {
              avoidcounts[i]--;
              write = false;
            }
          }
        }
        else
        {
          avoidcounts[i] = 0;
          newspeed = 200;
          
          /* update turnrate every 3 seconds */
          if(!(randcounts[i]))
          {
            if(modes[i] == STRAIGHT)
            {
              modes[i] = TURN;
              randcounts[i] = 20;

              randint = rand() % 40 - 20;
              newturnrate = randint;
            }
            else
            {
              modes[i] = STRAIGHT;
              randcounts[i] = 70;
              newturnrate = 0;
            }
          }
          else
          {
            write = false;
            randcounts[i]--;
          }
        }

        /* write commands to robot */
        //printf("writing (%d,%d) to client %d\n", 
               //newspeed, newturnrate, i);
        if(write)
          pproxies[i]->SetSpeed(newspeed,newturnrate);

        clients[i]->fresh = false;
      }
    }
  }

  return(0);
}
    

