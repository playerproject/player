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
      
      for( int a=0; a<3; a++ )
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
      
    }
}


