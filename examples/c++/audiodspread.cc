#include <playerclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#define AFMT_S16_LE 0x00000010

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
  AudioDSPProxy ap(&robot,0,'r');
  if(ap.Configure(1, 8000, AFMT_S16_LE))
    {
    printf("audiodsp configure failed\n");
    exit(1);
    }

  for(;;)
  {
    if(robot.Read())
      exit(1);
    for (int i = 0; i < 5; i++){
      printf("freq: %8d |", ap.freq[i]);
      for (int j = 0; j < ((int) (ap.amp[i] / 1000)); j++)
        printf("*");
      printf("\n"); 
      }
    printf("\n"); 
  }

  return(0);
}

