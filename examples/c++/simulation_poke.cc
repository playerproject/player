/*  
    File: simulation_poke.cc
    
    Author: Richard Vaughan (vaughan@sfu.ca)
    
    Created: 7 Feb 2005
    
    Description: Demonstrates how to use the SimulationProxy to move an
                 object in a simulator. Getting the object's identifying
		 string (name) is simulation-dependent. In Stage, the name
		 is specified in the worldfile.
    
    License: GNU GPL v.2 or later

    $Id$
*/

#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <limits.h>  // for INT_MAX
#include <string.h> /* for strcmp() */
#include <unistd.h> /* for usleep() */

#define USAGE \
  "USAGE: simulation_poke [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n" \
  "       -r <string>: identifier string for the object to move\n" \

char host[256] = "localhost";
char simobject[256] = "robot";
int port = PLAYER_PORTNUM;
int device_index = 0; // use this to access the nth indexed position and laser devices

/* easy little command line argument parser */
void
parse_args(int argc, char** argv)
{
  int i;

  i=1;
  while(i<argc)
  {
    if(!strcmp(argv[i],"-r"))
    {
      if(++i<argc)
        strcpy(simobject,argv[i]);
      else
      {
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-h"))
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
    else if(!strcmp(argv[i],"-i"))
    {
      if(++i<argc)
        device_index = atoi(argv[i]);
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

  PlayerClient robot(host,port);
  SimulationProxy sp(&robot,device_index,'w');

  printf("%s\n",robot.conn.banner);

  if(sp.access != 'w')
    {
      puts( "can't write to simulator" );
      exit(-1);
    }  

  double width = 10;
  double height = 10;
  
  printf( "Moving simulation object \"%s\" to random poses\n",
	  simobject );
  
  for(;;)
    {
      // move the named object to a random position about the origin
      sp.SetPose2D( simobject, 
		  -width/2.0 + drand48() * width, 
		    -height/2.0 + drand48() * height, 
		    drand48() * M_PI * 2.0 );

      sleep(1);
  }
}
