#include <playercclient.h>
#include <stdlib.h>
#include <stdio.h>

#define USAGE "USAGE: simple [-h <host>] [-p <port>]"

int 
main(int argc, char** argv)
{
  player_connection_t conn;
  char host[256] = "localhost";
  int port = PLAYER_PORTNUM;
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
    else
    {
      puts(USAGE);
      exit(1);
    }
    i++;
  }

  if(player_connect(&conn, host, port) == -1)
    exit(1);

  printf("Connected to: %s\n", conn.banner);
  if(player_disconnect(&conn))
    exit(1);

  return(0);
}
