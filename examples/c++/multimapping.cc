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

void sig_int(int num)
{
  struct timeval curr;
  gettimeofday(&curr,NULL);
  printf("# stopped at %s\n", ctime(&(curr.tv_sec)));
  fprintf(stderr,"# stopped at %s\n", ctime(&(curr.tv_sec)));
  fflush(NULL);
  exit(0);
}

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

#define IMG_WIDTH 3117
#define IMG_HEIGHT 1189
#define PPM 12.0

int main(int argc, char** argv)
{
  int randint;
  unsigned short minfrontdistance = 400;
  bool obs = false;
  bool write = false;

  char* occ_grid = new char[IMG_WIDTH*IMG_HEIGHT];

  bzero(occ_grid,IMG_HEIGHT*IMG_WIDTH);
  
  // to flush logs
  signal(SIGINT,sig_int);
  signal(SIGTERM,sig_int);
  signal(SIGHUP,sig_int);

  // log header stuff
  puts("# Occupancy grid");
  printf("# %s running %d robots\n", host,numclients);
  struct timeval curr;
  gettimeofday(&curr,NULL);
  printf("# started at %s\n", ctime(&(curr.tv_sec)));
  puts("# format (seconds and meters):");
  puts("# t sx sy ");
  
  // log header stuff
  fputs("# Robot position log\n",stderr);
  fprintf(stderr,"# %s running %d robots\n", host,numclients);
  fprintf(stderr,"# started at %s\n", ctime(&(curr.tv_sec)));
  fputs("# format (seconds and meters and degrees):\n",stderr);
  fputs("# t r0x r0y r0th .....\n",stderr);

  /* first, parse command line args */
  parse_args(argc,argv);

  // seed the RNG
  gettimeofday(&curr,NULL);
  srand(curr.tv_usec);

  PlayerMultiClient mc;

  PlayerClient** clients = new PlayerClient*[numclients];
  PositionProxy** pproxies = new PositionProxy*[numclients];
  SonarProxy** sproxies = new SonarProxy*[numclients];
  GpsProxy** gproxies = new GpsProxy*[numclients];

  int* randcounts = new int[numclients];
  int* avoidcounts = new int[numclients];
  int* modes = new int[numclients];

  bzero(randcounts,sizeof(int)*numclients);
  bzero(avoidcounts,sizeof(int)*numclients);
  bzero(modes,sizeof(int)*numclients);

  for(int i=0;i<numclients;i++)
  {
    clients[i] = new PlayerClient(host,baseport+i);
    mc.AddClient(clients[i]);

    pproxies[i] = new PositionProxy(clients[i],0,'a');
    sproxies[i] = new SonarProxy(clients[i],0,'r');
    gproxies[i] = new GpsProxy(clients[i],0,'r');
  }

  int newturnrate=0,newspeed=0;

  if(mc.Read())
    exit(1);
  if(mc.Read())
    exit(1);
  if(mc.Read())
    exit(1);

  int count = 0;
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
        // every so often, log robot positions
        // at 10Hz, 50 cycles is 5 sec
        if((i == 0) && !(++count % 50))
        {
          fprintf(stderr,"%f ",
                    gproxies[i]->timestamp.tv_sec + 
                    gproxies[i]->timestamp.tv_usec / 1000000.0);
          for(int j=0;j<numclients;j++)
          {
            fprintf(stderr,"%.3f %.3f %d ",
                    gproxies[j]->xpos/1000.0,
                    gproxies[j]->ypos/1000.0,
                    RTOD(NORMALIZE(DTOR(gproxies[j]->heading))));
          }
          fputs("\n",stderr);
          count = 0;
        }

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

        // transform sonar hit points to global coord
        for(int j=0;j<PLAYER_NUM_SONAR_SAMPLES;j++)
        {
          if(sproxies[i]->ranges[j] >= 5000)
            continue;
          // first to robot coord
          // find where the sonar is
          double px,py,pth;
          sproxies[i]->GetSonarPose(j,&px,&py,&pth);
          double rx = px + (sproxies[i]->ranges[j]/1000.0)*cos(pth);
          double ry = py + (sproxies[i]->ranges[j]/1000.0)*sin(pth);

          //gproxies[i]->Print();
          // now to global
          double gx = gproxies[i]->xpos/1000.0 + 
                  rx*cos(DTOR(gproxies[i]->heading)) - 
                  ry*sin(DTOR(gproxies[i]->heading));
          double gy = gproxies[i]->ypos/1000.0 +
                  rx*sin(DTOR(gproxies[i]->heading)) +
                  ry*cos(DTOR(gproxies[i]->heading));

          int occ_x = (int)rint(gx * 12.0);
          int occ_y = (int)rint(gy * 12.0);

          if(!occ_grid[occ_x+occ_y*IMG_WIDTH])
          {
            occ_grid[occ_x+occ_y*IMG_WIDTH] = 1;

            printf("%f %f %f\n", 
                   sproxies[i]->timestamp.tv_sec + 
                   sproxies[i]->timestamp.tv_usec / 1000000.0,
                   gx, gy);
          }
        }

        clients[i]->fresh = false;
      }
    }
  }

  return(0);
}
    

