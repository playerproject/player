#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */

#define USAGE \
  "USAGE: stage [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" 

char host[256] = "localhost";
int port = PLAYER_PORTNUM;

/* easy little command line argument parser */
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
  int create = 0;

  parse_args(argc,argv);

  // connect to Player
  PlayerClient pclient(host,port);

  // access the Truth device
  StageProxy sp( &pclient, 0, 'a' );

  /* go into read-think-act loop */
  for(;;)
  {
    puts( "reading..." );
    /* this blocks until new data comes; 10Hz by default */
    if( pclient.Read())
      exit(1);

    puts( "printing" );
    /* print data to console */
    sp.Print();

    if( create == 0 )
      {
	if( sp.CreateModel( "box", "my box", NULL, 1.0, 2.0, 0.0 ) != 0 )
	  puts( "Error creating model" );

	create = 1;
      }
  }
}



