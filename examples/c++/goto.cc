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

#define radians(deg) deg*(M_PI/180.0)
#define degrees(rad) rad*(180.0/M_PI)

#define min(a,b) (a < b) ? a : b
#define max(a,b) (a > b) ? a : b
#define sign(a) (a >= 0.0) ? 1 : -1

#define USAGE \
  "USAGE: goto [-x <x>] [-y <y>] [-h <host>] [-p <port>] [-m]\n" \
  "       -x <x>: set the X coordinate of the target to <x>\n"\
  "       -y <y>: set the Y coordinate of the target to <y>\n"\
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -m       : turn on motors (be CAREFUL!)"

CRobot robot;
bool turnOnMotors = false;
char host[256] = "localhost";
pthread_mutex_t dataAccessMutex;
pthread_mutex_t commandAccessMutex;

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
    dist = sqrt((target.x - robot.position->xpos)*
                (target.x - robot.position->xpos) + 
                (target.y - robot.position->ypos)*
                (target.y - robot.position->ypos));
    angle = 
       atan2(target.y - robot.position->ypos,target.x-robot.position->xpos);
    pthread_mutex_unlock(&dataAccessMutex);

    //printf("dist:%f\tangle:%f\n", dist,degrees(angle));

    pthread_mutex_lock(&commandAccessMutex);
    if(fabs(degrees(angle)) > 10.0)
    {
      robot.newturnrate = (short)((angle/M_PI) * 40.0);
      robot.newturnrate = (short)(min(robot.newturnrate,40.0));
      robot.newturnrate = (short)(max(robot.newturnrate,-40.0));
    }
    else
      robot.newturnrate = (short)0.0;

    if(dist > 50.0)
    {
      robot.newspeed = (short)((dist/500.0) * 200.0);
      robot.newspeed = (short)(min(robot.newspeed,200.0));
      robot.newspeed = (short)(max(robot.newspeed,-200.0));
    }
    else
      robot.newspeed = (short)0.0;

    if(!robot.newspeed)
      gotodone = 1;

    pthread_mutex_unlock(&commandAccessMutex);

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

  for(;;)
  {
    //puts("obs");
    pthread_mutex_lock(&dataAccessMutex);
    pthread_mutex_lock(&commandAccessMutex);
    //puts("obs locked commandAccessMutex");
    avoid = 0;

    if((robot.sonar[2] < really_min_front_dist) ||
       (robot.sonar[3] < really_min_front_dist) ||
       (robot.sonar[4] < really_min_front_dist) ||
       (robot.sonar[5] < really_min_front_dist))
    {
      avoid = 1;
      //puts("AVOID");
      robot.newspeed = -100;
    }
    else if((robot.sonar[2] < min_front_dist) ||
            (robot.sonar[3] < min_front_dist) ||
            (robot.sonar[4] < min_front_dist) ||
            (robot.sonar[5] < min_front_dist))
    {
      //puts("AVOID");
      //avoid = 1;
      robot.newspeed = 0;
    }

    if(avoid)
    {
      if((robot.sonar[0] + robot.sonar[1]) < (robot.sonar[6] + robot.sonar[7]))
	robot.newturnrate = 30;
      else
	robot.newturnrate = -30;
    }

    pthread_mutex_unlock(&commandAccessMutex);
    //puts("obs unlocked commandAccessMutex");
    pthread_mutex_unlock(&dataAccessMutex);

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
        strcpy(robot.host,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        robot.port = atoi(argv[i]);
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
  pthread_mutex_init( &commandAccessMutex, NULL );

  parse_args(argc,argv);

  /* Connect to Player server */
  if(robot.Connect())
    exit(0);

  /* Request sensor data */
  if(robot.Request( "pasr" , 4))
    exit(0);
    
  if(turnOnMotors && robot.ChangeMotorState(1))
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
    if(robot.Read())
      exit(1);
    
    //robot.Print();

    pthread_mutex_unlock(&dataAccessMutex);
    pthread_mutex_lock(&commandAccessMutex);

    robot.Write();
    if(gotodone)
      break;
    pthread_mutex_unlock(&commandAccessMutex);
  }
    
  /* close everything */
  robot.Request( "pcsc" , 4 );
  return(0);
}

