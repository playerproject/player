#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */
#include <assert.h>
#include <math.h>

/* 
 * $Id$
 */

#define USAGE \
  "USAGE: swarmbot [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n"

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;

const char MESSAGE_BYTES[4] = {0xFF, 0x00, 0xFF, 0x11};
const int MESSAGE_LEN = 4;

int num_robots = 1;

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
    else if(!strcmp(argv[i],"-n"))
    {
      if(++i<argc)
        num_robots = atoi(argv[i]);
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
  
  printf( "starting %d swarbots\n", num_robots );

  //PlayerClient *robots = malloc( num_robots * sizeof(PlayerClient*) );
  
  //for( int r=0; r<num_robots; r++ )
  //robots[r] = 

  PlayerClient robot(host,port);
  robot.SetFrequency( 10 ); // request data at 10Hz
  
  FiducialProxy **fps = (FiducialProxy**)calloc( num_robots, sizeof(FiducialProxy*) ); 
  PositionProxy **pps = (PositionProxy**)calloc( num_robots, sizeof(PositionProxy*) ); 

  
  for( int r=0; r<num_robots; r++ )
    {
      fps[r] = new FiducialProxy(&robot,r,'a');
      pps[r] = new PositionProxy(&robot,r,'a');
    }
  
  // try a few reads
  for( int a=0; a<10; a++ )
    if(robot.Read()) exit(1);    

  while( 1 )
    {
      // wait for the playerclient to get new data. each read we have
      // a new array of visible neigbors, with their angles and ranges
      if(robot.Read()) exit(1);
      

      for( int r=1; r<num_robots; r++ )
	{
	  // a little potential field algorithm for dispersal
	  double dx=0.0, dy=0.0;
	  
	  double ax=0.0, ay=0.0;

	  // for each neighbor detected
	  for( int s=0; s< fps[r]->count; s++ )
	    {
	      double range = fps[r]->beacons[s].pose[0] / 1000.0;
	      double bearing = DTOR(fps[r]->beacons[s].pose[1]);   

	      range = range - 1.0;
	      
	      dx += range * cos( bearing ); 	  
	      dy += range * sin( bearing ); 	   

	      // also compute the average global heading
	      ax += cos( DTOR(fps[r]->beacons[s].pose[2] ) ); 
	      ay += sin( DTOR(fps[r]->beacons[s].pose[2] ) ); 
	    }

	  // find the average heading 
	  //double angle = DTOR(pps[r]->theta);
	  int turnrate_deg_sec = 0;
	  
	  // normalize
	  //while( angle > M_PI )
	  //angle -= 2.0 * M_PI;
	  
	  //while( angle < -M_PI )
	  //angle += 2.0 * M_PI;

	  // find the error
	  double angle = atan2( ay, ax );	  

	  printf( "theta error %.2f\n", angle );

	  turnrate_deg_sec = 10 * (int)RTOD( angle );
	  //if( angle >  0.1 ) turnrate_deg_sec = 45;
	  //if( angle < -0.1 ) turnrate_deg_sec = -45;
	  
	  // turn towards the average heading

	  int xspeed = (int)( 500.0 * dx ); 
	  int yspeed = (int)( 500.0 * dy ); 
	  
	  int wspeed = (int)( 500.0 * hypot(dy,dx) );

	  printf( "dx: %.2f  dy: %.2f  head: %.2f  -- %d %d %d\n", 
		  dx, dy, angle, xspeed, yspeed, turnrate_deg_sec );
	  
	  
	  pps[r]->SetSpeed( xspeed, yspeed, turnrate_deg_sec );
	  //pps[r]->SetSpeed( 0, 0, turnrate_deg_sec );
	}
    }
}


