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


/*
typedef struct
{

}
*/

int main(int argc, char **argv)
{
  parse_args(argc,argv);
  
  printf( "starting %d swarbots\n", num_robots );

  PlayerClient robot(host,port);
  robot.SetFrequency( 2 ); // request data at 10Hz
  
  FiducialProxy **fps = 
    (FiducialProxy**)calloc( num_robots, sizeof(FiducialProxy*) ); 

  PositionProxy **pps = 
    (PositionProxy**)calloc( num_robots, sizeof(PositionProxy*) ); 

  
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
      
      for( int r=0; r<num_robots; r++ )
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
	  int speed_mm_sec = 0;

	  // error to the average angle of my buddies
	  double buddy_error_angle = atan2( ay, ax );	  

	  double position_error_angle = atan2( dy, dx );
	  double position_error_distance = hypot( dy, dx );

	  // if we're far from the ideal position
	  if( fabs(position_error_distance) > 0.2 )
	    {
	      // minimize the position angle error
	      turnrate_deg_sec = 2 * (int)RTOD(position_error_angle);
	      
	      // if we're pointing the right direction, move forwards
	      if( fabs(position_error_angle) < 0.1 )
		{
		  // speed propotional to error
		  speed_mm_sec = (int)( 500.0 * position_error_distance );
		  if( speed_mm_sec < 100 ) 
		    speed_mm_sec = 100;
		}
	    }
	  else // we're close to where we want to be, so now point in the 
	    {
	      // average direction
	      turnrate_deg_sec = 2 * (int)RTOD(buddy_error_angle);

	      // if we're in just the right place, announce it
	      if( fabs(buddy_error_angle) < 0.05 ) 
		{
		  player_fiducial_msg_t msg;
		  
		  msg.target_id = -1;
		  strcpy( (char*)msg.bytes, "Aligned" );
		  msg.len = strlen( (char*)msg.bytes );
		  msg.intensity = 200;
		  
		  fps[r]->SendMessage( &msg, true );
		}
	    }
	  
	  //printf( "position_error_distance %.2f  "
	  //  " position_error_angle %.2f  buddy_error_angle %.2f\n", 
	  //  position_error_distance,
	  //  position_error_angle,
	  //  buddy_error_angle );
	  
	  //printf( "%d %d\n", 
	  //  speed_mm_sec, turnrate_deg_sec );
	
	  pps[r]->SetSpeed( speed_mm_sec, turnrate_deg_sec );
	}
    }
}


