/*
 * say.cc
 *
 * a simple demo to show how to send commands to the Festival speech
 * synthesis device
 *
 */

#define USAGE "USAGE: say [-h <host>] [-p <port>] <string>"

#include <playerclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

char host[256] = "localhost";
int port = PLAYER_PORTNUM;
char str[256];

/* parse command-line args */
void
parse_args(int argc, char** argv)
{
  int i;

  if(argc < 2)
  {
    puts(USAGE);
    exit(1);
  }

  strcpy(str,argv[argc-1]);

  i=1;
  while(i<argc-1)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i<argc-1)
        strcpy(host,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i<argc-1)
        port = atoi(argv[i]);
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

int main(int argc, char **argv)
{ 
  parse_args(argc,argv);

  /* Connect to Player server */
  PlayerClient robot(host,port);

  /* Request sensor access */
  SpeechProxy fp(&robot,0,'w');

  /* send the string */
  for(;;)
  {
    printf("Saying \"%s\"...\n", str);
    fp.Say(str);
    usleep(100000);
  }

  /* wait a little to make sure that it gets out to the sound card */
  sleep(5);

  return(0);
}

