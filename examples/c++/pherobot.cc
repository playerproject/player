#include <stdio.h>
#include <stdlib.h>  // for atoi(3)
#include <playerclient.h>  // for player client stuff
#include <string.h> /* for strcpy() */
#include <assert.h>

#define USAGE \
  "USAGE: pherobot [-h <host>] [-p <port>] [-m]\n" \
  "       -h <host>: connect to Player on this host\n" \
  "       -p <port>: connect to Player on this TCP port\n"

bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;

const char MESSAGE_BYTES[4] = {0xFF, 0x00, 0xFF, 0x11};
const int MESSAGE_LEN = 4;

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

#define NUM_IDARS 8

int main(int argc, char **argv)
{
  //unsigned short min_front_dist = 500;
  //unsigned short really_min_front_dist = 300;
  //char avoid;

  parse_args(argc,argv);

  PlayerClient robot(host,port);
  robot.SetFrequency( 5 ); // request data at 5Hz
  
  DescartesProxy dp(&robot,0,'a');

  IDARProxy *ip[ NUM_IDARS ];

  for( int i=0; i < NUM_IDARS; i++ )
    assert( ip[i] = new IDARProxy(&robot,i,'a') );

  // try a few reads
  for( int a=0; a<5; a++ )
    if(robot.Read()) exit(1);    
    
  short speed = 100;     // mm/sec
  short heading = 0;     // degrees
  short distance = 1000; // mm

  idartx_t send_msg;
  idarrx_t recv_msg;

  // compose a message
  memcpy( &send_msg.mesg, &MESSAGE_BYTES, MESSAGE_LEN );
  send_msg.len = MESSAGE_LEN;
  send_msg.intensity = 50;


  for( int g=0; /* loop forever */ ; g++)
  {
    
    // send the message from all idars
    for( int i=0; i < NUM_IDARS; i++ )
      if( ip[i]->SendMessage( &send_msg ) < 0 )
	puts( "idar send failed" );
    
    // fetch received messages
    for( int i=0; i < NUM_IDARS; i++ )
      {
	if( ip[i]->GetMessage( &recv_msg ) < 0 )
	  puts( "idar receive failed" );
	
	ip[i]->PrintMessage( &recv_msg );
      }

    // wait for the playerclient to get new data
    if(robot.Read()) exit(1);
 
    dp.Print();

    
    /* write occasional commands to robot */
    if( (g % 5) == 0 )
      dp.Move( speed, heading=((heading+100)%360), distance );
  }
}

