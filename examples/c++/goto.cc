/*
 * goto.cc - a simple (and bad) goto program
 *
 * but, it demonstrates one multi-threaded structure
 */

#include <stdio.h>
#include <stdlib.h> // for atof()
#include <unistd.h>
#include <playerclient.h>
#include <pthread.h>
#include <math.h>
#include <string.h> /* for strcmp() */
#include <values.h>  // for MAXINT

#define radians(deg) deg*(M_PI/180.0)
#define degrees(rad) rad*(180.0/M_PI)

#define USAGE \
  "USAGE: goto [-x <x>] [-y <y>] [-h <host>] [-p <port>] [-m]\n" \
  "       -x <x>: set the X coordinate of the target to <x>\n"\
  "       -y <y>: set the Y coordinate of the target to <y>\n"\
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

PlayerClient* robot;
PositionProxy* pp;
FRFProxy* sp;

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;
pthread_mutex_t dataAccessMutex;

typedef struct
{
  float x;
  float y;
} pos_t; 

pos_t target;
bool gotodone = false;

/*
 * very bad goto.  target is arg (as pos_t*)
 *
 * sets global 'gotodone' when it's done
 */
void*
GotoThread(void* arg)
{
  pos_t target = *(pos_t*)arg;
  float dist, angle;

  printf("GotoThread starting. target: (%.2f,%.2f)\n", target.x,target.y);
  for(;;)
  {
    //puts("goto");
    //printf("target: (%f,%f)\n", target.x, target.y);
    
    pthread_mutex_lock(&dataAccessMutex);
    dist = sqrt((target.x - pp->xpos)*
                (target.x - pp->xpos) + 
                (target.y - pp->ypos)*
                (target.y - pp->ypos));
    angle = 
       atan2(target.y - pp->ypos,target.x-pp->xpos);
    pthread_mutex_unlock(&dataAccessMutex);

    //printf("dist:%f\tangle:%f\n", dist,degrees(angle));

    int newturnrate,newspeed;
    if(fabs(degrees(angle)) > 10.0)
    {
      newturnrate = (short)((angle/M_PI) * 40.0);
      newturnrate = (short)(min(newturnrate,40.0));
      newturnrate = (short)(max(newturnrate,-40.0));
    }
    else
      newturnrate = (short)0.0;

    if(dist > 50.0)
    {
      newspeed = (short)((dist/500.0) * 200.0);
      newspeed = (short)(min(newspeed,200.0));
      newspeed = (short)(max(newspeed,-200.0));
    }
    else
      newspeed = (short)0.0;

    if(!newspeed)
      gotodone = 1;

    pp->SetSpeed(newspeed, newturnrate);

    usleep(200000);
  }
}

/*
 * sonar avoid.
 *   policy:
 *     if(object really close in front)
 *       backup and turn away;
 *     else if(object close in front)
 *       stop
 */
void*
SonarObstacleAvoidThread(void* arg)
{
  unsigned short min_front_dist = 500;
  unsigned short really_min_front_dist = 300;
  char avoid;

  puts("ObstacleAvoidThread starting");

  int newturnrate, newspeed;
  for(;;)
  {
    newspeed = newturnrate = MAXINT;
    
    //puts("obs");
    pthread_mutex_lock(&dataAccessMutex);
    avoid = 0;

    if((sp->ranges[2] < really_min_front_dist) ||
       (sp->ranges[3] < really_min_front_dist) ||
       (sp->ranges[4] < really_min_front_dist) ||
       (sp->ranges[5] < really_min_front_dist))
    {
      avoid = 1;
      //puts("AVOID");
      newspeed = -100;
    }
    else if((sp->ranges[2] < min_front_dist) ||
            (sp->ranges[3] < min_front_dist) ||
            (sp->ranges[4] < min_front_dist) ||
            (sp->ranges[5] < min_front_dist))
    {
      //puts("AVOID");
      avoid = 1;
      newspeed = 0;
    }

    if(avoid)
    {
      if((sp->ranges[0] + sp->ranges[1]) < (sp->ranges[6] + sp->ranges[7]))
	newturnrate = 30;
      else
	newturnrate = -30;
    }

    pthread_mutex_unlock(&dataAccessMutex);

    if(newspeed < MAXINT && newturnrate < MAXINT)
      pp->SetSpeed(newspeed, newturnrate);

    usleep(100000);
  }
}
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
    else if(!strcmp(argv[i],"-y"))
    {
      if(++i<argc)
        target.y = atof(argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-x"))
    {
      if(++i<argc)
        target.x = atof(argv[i]);
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

/* helper routine to make the main code look cleaner */
int
startthread(void * (*name)(void *), void* arg)
{
  pthread_t thread;   
  pthread_attr_t attr;

  return(pthread_attr_init(&attr) ||
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) ||
         pthread_create(&thread, &attr, name, arg));
}


int main(int argc, char **argv)
{
  target.x = 0.0;
  target.y = 0.0;

  /* the mutex's are defined to be default type (NULL) which is fast */
  pthread_mutex_init( &dataAccessMutex, NULL );

  parse_args(argc,argv);

  /* Connect to Player server */
  robot = new PlayerClient(host,port);

  /* Request sensor data */
  pp = new PositionProxy(robot,0,'a');
  sp = new FRFProxy(robot,0,'r');
    
  if(turnOnMotors && pp->SetMotorState(1))
    exit(0);

  if(startthread(&SonarObstacleAvoidThread,NULL))
  {
    fputs("pthread creation for SonarObstacleAvoidThread failed. bailing.\n", stderr);
    exit(1);
  }
  if(startthread(&GotoThread,&target))
  {
    fputs("pthread creation for GotoThread failed. bailing.\n", stderr);
    exit(1);
  }

  for(;;)
  {
    pthread_mutex_lock(&dataAccessMutex);
    if(robot->Read())
      exit(1);
    
    pp->Print();
    sp->Print();

    pthread_mutex_unlock(&dataAccessMutex);

    if(gotodone)
      break;
  }
    
  return(0);
}

