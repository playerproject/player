/*
 * lookupport.cc
 *
 * an example/utility that looks up the port for a robot with a given name.
 * only really useful with Stage.
 */

#include <playerclient.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define USAGE \
  "USAGE: lookup [-h <host>] [-p <port>] <name>\n" \
  "       -h <host> : connect to Player on this host\n" \
  "       -p <port> : connect to Player on this TCP port\n" \
  "          <name> : lookup this robot name\n"

char host[256] = "localhost";
int port = PLAYER_PORTNUM;
char robotname[64];

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

  i=1;
  while(i<argc-1)
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
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }
  strncpy(robotname,argv[i],sizeof(robotname));
}

int main(int argc, char **argv)
{ 
  int robotport;
  parse_args(argc,argv);
  PlayerClient robot;

  if(robot.ConnectRNS(robotname,host,port) < 0)
    exit(-1);

  for(;;)
    robot.Read();

  //PlayerClient robot(host,port);
  //robotport = robot.LookupPort(robotname);
  //printf("Robot \"%s\" is on port %d\n", robotname, robotport);

  return(0);
}

