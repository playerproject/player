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
  BlinkenlightProxy* bpr, *bpg, *bpb;
  SonarProxy* sp;

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
      bots[r].bpr = new BlinkenlightProxy(bots[r].pc,0,'a');
      bots[r].bpg = new BlinkenlightProxy(bots[r].pc,1,'a');
      bots[r].bpb = new BlinkenlightProxy(bots[r].pc,2,'a');

      //bots[r].sp = new SonarProxy(bots[r].pc,0,'a');

      // create some dummy messages in the store
      for( int s=0; s<storelen; s++ )
	{
	  bots[r].store[s].sender = 0;
	  bots[r].store[s].range = 10000;
	}   
      
      for( int a=0; a<5; a++ )
	if( bots[r].pc->Read()) exit(1);
 
      //bots[r].sp->GetSonarGeom();
   }
  

  // blink the zeroth robot's blinkenlight
  //bots[0].bp->SetLight( true, 500 );
  
  while( 1 )
    {
      for( int r=0; r<num_robots; r++ )
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
	      double range = (bots[r].fp->beacons[s].pose[0] / 1000.0);
	      double bearing = DTOR(bots[r].fp->beacons[s].pose[1]);   
	      
	      range = 1.0 / (range*range);

	      //range = range - 1.5;
	      
	      dx -= range * cos( bearing ); 	  
	      dy -= range * sin( bearing ); 	   
	    }
	 
	  /*
	  // for each sonar reading
	  for( int s=0; s< bots[r].sp->range_count; s++ )
	    {
	      double range = bots[r].sp->ranges[s] / 1000.0;
	      double bearing = DTOR(bots[r].sp->sonar_pose.poses[s][2]);   
	      
	      //range = range - 1.5;
	      
	      //if( range < 0.5 ) range = 0.0;

	      dx += range * cos( bearing ); 	  
	      dy += range * sin( bearing ); 	   
	    }
	  */

	  // threshold our turnrate
	  int turnrate = direction;
	  if( turnrate > 100 ) turnrate = 100.0;
	  if( turnrate < -100 ) turnrate = -100.0;
	  
	  // move!
	  //bots[r].pp->SetSpeed( 200*dx, 200*dy, turnrate );
	  //bots[r].pp->SetSpeed( 200*dx, 200*dy, 0 );
	  
	  double resultant_angle = atan2(dy,dx);
	  double resultant_distance = hypot(dx,dy );

	  int forwards = 0;
	  if( fabs(resultant_angle) < 0.1 )
	    forwards = (int)(20 * resultant_distance);
	  
	  if( fabs(forwards) < 25 ) forwards = 0;
	  if( forwards > 400 ) forwards = 400;
	  if( forwards < -400 ) forwards = -400;


	  //bots[r].pp->SetSpeed( 0,0, 2 * (int)RTOD(resultant_angle)  );
	  bots[r].pp->SetSpeed( forwards,0, 2.0 * (int)RTOD(resultant_angle)  );

	  //printf( "dx %.2f dy %.2f  = %.2f radians %.2fm  forwards %d\n",
	  //  dx, dy, atan2(dy,dx), hypot(dx,dy), forwards );



	  // enable the blue light if we're pointing to the source
	  if( fabs(direction) < 3 )
	    bots[r].bpb->SetLight( true, 0 );
	  else
	    bots[r].bpb->SetLight( false, 0 );


	  // enable the red light if we're happy with our position
	  if( forwards == 0 )
	    bots[r].bpr->SetLight( true, 0 );
	  else
	    bots[r].bpr->SetLight( false, 0 );

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


