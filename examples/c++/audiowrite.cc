#include <playerclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char host[256] = "localhost";
int port = PLAYER_PORTNUM;

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
        //puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc)
        port = atoi(argv[i]);
      else
      {
        //puts(USAGE);
        exit(1);
      }
    }
    else
    {
      //puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{ 
  parse_args(argc,argv);

  /* Connect to Player server */
  PlayerClient robot(host,port);

  /* Request sensor data */
  AudioProxy ap(&robot,0,'w');

  for(;;)
  {
    if(robot.Read())
      exit(1);

    ap.PlayTone(3000,40,5);
  }

  return(0);
}

