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

/* command line argument parser */
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

typedef struct
{
  char type;
  double range;
} gradient_t;

typedef struct
{
  int sender;
  int range;
} grad_direction_t;

#define storelen 10

typedef struct
{
  PlayerClient* pc;
  FiducialProxy* fp;
  PositionProxy* pp;
  BlinkenlightProxy* bp;
  
  grad_direction_t store[storelen];
} swarmbot_t;


int main(int argc, char **argv)
{
  parse_args(argc,argv);
  
  printf( "starting %d swarbots\n", num_robots );

  swarmbot_t *bots = (swarmbot_t*)calloc( num_robots, sizeof(swarmbot_t) );

  for( int r=0; r<num_robots; r++ )
    {  
      bots[r].pc = new PlayerClient(host,port++);      
      bots[r].fp = new FiducialProxy(bots[r].pc,0,'a');
      bots[r].pp = new PositionProxy(bots[r].pc,0,'a');
      bots[r].bp = new BlinkenlightProxy(bots[r].pc,0,'a');

      // create some dummy messages in the store
      for( int s=0; s<storelen; s++ )
	{
	  bots[r].store[s].sender = 0;
	  bots[r].store[s].range = 10000;
	}   
      
      for( int a=0; a<5; a++ )
	if( bots[r].pc->Read()) exit(1);
    }
  
  // blink the zeroth robot's blinkenlight
  bots[0].bp->SetLight( true, 500 );
  
  while( 1 )
    {
      // robot zero sends out a message with hop count zero
      gradient_t grad;
      grad.type = 1;
      grad.range = 0;
      
      // build a message incorporating the grad packet
      player_fiducial_msg_t msg;		  
      msg.target_id = -1;
      memcpy(msg.bytes,&grad,sizeof(grad));
      msg.len = sizeof(grad);
      msg.intensity = 200;
      
      bots[0].pc->Read();
      bots[0].fp->SendMessage( &msg, true );
      
      for( int r=1; r<num_robots; r++ )
	{
	  // wait for the playerclient to get new data. each read we have
	  // a new array of visible neigbors, with their angles and ranges
	  bots[r].pc->Read();
      
	  int upstream_bot = 0;
	  
	  // any messages for me?
	  player_fiducial_msg_t recv;
	  while( bots[r].fp->RecvMessage( &recv, true ) == 0 )
	    {
	      int sender = recv.target_id;
	      
	      if( sender != -1 && recv.len == sizeof(gradient_t) ) 
		{
		  gradient_t *grad = (gradient_t*)recv.bytes;
		  
		  // shift the array one place to the right
		  for( int m=storelen-1; m > 0; m-- )
		    memcpy( &bots[r].store[m], &bots[r].store[m-1], 
			    sizeof(grad_direction_t) );
		  
		  // find the range of the fiducial with this id
		  double range = 0;
		  for( int f=0; f<bots[r].fp->count; f++ )
		    {
		      if( bots[r].fp->beacons[f].id == sender )
			{
			  bots[r].store[0].range = grad->range +  
			    bots[r].fp->beacons[f].pose[0];
			  bots[r].store[0].sender = sender;
			  break;
			} 
		    }
		}
	    }
	  
	  // now look in the store to find the smallest range
	  int lowrange = 10000;
	  for( int m=0; m <storelen; m++ )
	    {
	      if( bots[r].store[m].range < lowrange )
		{
		  lowrange = bots[r].store[m].range;
		  upstream_bot = bots[r].store[m].sender;
		}
	    }   
	  
	  // find the direction of the fiducial that sent us the
	  // lowest cumulative range
	  int direction = 0;
	  for( int f=0; f<bots[r].fp->count; f++ )
	    {
	      if( bots[r].fp->beacons[f].id == upstream_bot )
		direction = bots[r].fp->beacons[f].pose[1];  
	    } 
	  
	  // a little potential field algorithm for dispersal
	  double dx=0.0, dy=0.0;
	  
	  // for each neighbor detected
	  for( int s=0; s< bots[r].fp->count; s++ )
	    {
	      double range = bots[r].fp->beacons[s].pose[0] / 1000.0;
	      double bearing = DTOR(bots[r].fp->beacons[s].pose[1]);   
	      
	      range = range - 1.5;
	      
	      dx += range * cos( bearing ); 	  
	      dy += range * sin( bearing ); 	   
	    }
	  
	  // threshold our turnrate
	  int turnrate = direction;
	  if( turnrate > 100 ) turnrate = 100.0;
	  if( turnrate < -100 ) turnrate = -100.0;
	  
	  // move!
	  bots[r].pp->SetSpeed( 200*dx, 200*dy, turnrate );

	  // enable the light if we're pointing to the source
	  if( fabs(direction) < 3 )
	    bots[r].bp->SetLight( true, 0 );
	  else
	    bots[r].bp->SetLight( false, 0 );

	  // transmit my range estimate to the broadcast address (-1)
	  if( lowrange < 10000 )
	    {
	      gradient_t grad;
	      grad.type = 1;
	      grad.range = lowrange;
	      
	      player_fiducial_msg_t send;
	      send.target_id = -1;
	      memcpy(send.bytes,&grad,sizeof(grad));
	      send.len = sizeof(grad);
	      send.intensity = 200;
	      
	      bots[r].fp->SendMessage( &send, true );
	    }
	}
    }
}


