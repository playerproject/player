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


int main(int argc, char **argv)
{
  parse_args(argc,argv);
  
  PlayerClient robot(host,port);
  robot.SetFrequency( 5 ); // request data at 5Hz
  
  DescartesProxy dp(&robot,0,'a');
  IDARTurretProxy ip(&robot,0,'a');
  
  // try a few reads
  for( int a=0; a<5; a++ )
    if(robot.Read()) exit(1);    

  // radians per sensor
  double dth = (M_PI*2.0)/(double)PLAYER_IDARTURRET_IDAR_COUNT;
  
  short speed = 200;     // mm/sec
  short heading = 0;     // degrees
  short distance = 1000; // mm
  
  idartx_t send_msg;
  
  // compose a message
  memcpy( &send_msg.mesg, &MESSAGE_BYTES, MESSAGE_LEN );
  send_msg.len = MESSAGE_LEN;
  send_msg.intensity = 50;
  
  // assemble some messages
  player_idarturret_config_t send_msgs;
  player_idarturret_reply_t recv_msgs;

  // copy the same message into all the send slots
  for( int i=0; i<PLAYER_IDARTURRET_IDAR_COUNT; i++ )
    memcpy( &send_msgs.tx[i], &send_msg, sizeof(send_msg) );
  
    
  struct timeval start_tv, end_tv;
  gettimeofday( &start_tv, NULL );

  int num_loops = 100;
  for( int g=0; /* loop forever */ ; g++)
    {
      if( ip.SendGetMessages( &send_msgs, &recv_msgs ) < 0 )
	puts( "idar send/get failed" );
      
      // wait for the playerclient to get new data
      if(robot.Read()) exit(1);
      
      // a little potential field algorithm for dispersal
      double dx=0.0, dy=0.0;
      
      for( int s=0; s<PLAYER_IDARTURRET_IDAR_COUNT; s++ )
	{
	  double intensity = (double)recv_msgs.rx[s].intensity;
	  double sensor_angle = dth * s;
	  
	  if( intensity < 100) intensity = 0;
	  
	  dx -= intensity * cos( sensor_angle ); 	  
	  dy -= intensity * sin( sensor_angle ); 	   
	}

      int resultant_angle =  (int)RTOD(atan2( dy, dx ));
      if( resultant_angle < 0 ) resultant_angle += 360; 
      heading = dp.theta + resultant_angle;
      distance = (int)hypot( dy, dx );
            
      dp.Move( speed, heading, distance ); // write commands to robot
    }
}


