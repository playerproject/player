/*
 * joystick.cc - control position and ptz devices with a joystick
 * 
 * $Id$
 */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h> /* for strcpy() */
#include <stdlib.h>  /* for atoi(3),rand(3) */
#include <playerclient.h>
#include <time.h>
#include <pthread.h>

#define USAGE \
  "USAGE: joystick [-h <host>] [-p <port>] n" \
  "       -h <host> : connect to Player on this host\n" \
  "       -p <port> : connect to Player on this TCP port\n" 


char host[256] = "localhost";
int port = PLAYER_PORTNUM;

/////////////////////////////////////////////////////////////////////
// this is the event structure from the linux joystick driver v2.0.0
//

struct js_event {
  uint32_t time;     /* event timestamp in milliseconds */
  int16_t value;    /* value */
  uint8_t type;      /* event type */
  uint8_t number;    /* axis/button number */
};

#define JS_EVENT_BUTTON         0x01    /* button pressed/released */
#define JS_EVENT_AXIS           0x02    /* joystick moved */
#define JS_EVENT_INIT           0x80    /* initial state of device */

#define XAXIS 0
#define YAXIS 1
#define XAXIS2 2
#define YAXIS2 3
#define XAXIS3 4
#define YAXIS3 5
#define XAXIS4 6
#define YAXIS4 7

// define the speed limits for the robot

// at full joystick depression you'll go this fast
#define MAX_SPEED    300 // mm/second
#define MAX_TURN    45 // degrees/second

// this is the speed that the camera will pan when you press the
// hatswitches in degrees/sec
#define PAN_SPEED 2
#define TILT_SPEED 2


// normalizing macros
#define AXIS_MAX            32767.0
#define PERCENT(X)          (X * 100.0 / AXIS_MAX)
#define PERCENT_POSITIVE(X) (((PERCENT(X)) + 100.0) / 2.0)
#define KNORMALIZE(X)       (((X * 1024.0 / AXIS_MAX)+1024.0) / 2.0)
#define NORMALIZE_SPEED(X)  ( X * MAX_SPEED / AXIS_MAX )
#define NORMALIZE_TURN(X)  ( X * MAX_TURN / AXIS_MAX )

struct controller
{
  int speed, turnrate, pan, tilt, zoom;
};


/* parse command-line args */
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
  }
}


// open a joystick device and read from it, scaling the values and
// putting them into the controller struct
void joystick_handler( struct controller* cont )
{
  // cast to a recognized type
  //struct controller* cont = (struct controller*)arg;

  int jfd = open ("/dev/js0", O_RDONLY);
  
  if( jfd < 1 )
    {
      perror( "failed to open joystick" );
      exit( 0 );
    }

  struct js_event event;
  
  int buttons_state = 0;

  // loop around a joystick read
  for(;;)
    {
      // get the next event from the joystick
      read (jfd, &event, sizeof(struct js_event));
      
      if ((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON) {
	if (event.value)
	  buttons_state |= (1 << event.number);
	else
	  buttons_state &= ~(1 << event.number);
      }
      
      //printf( "value % d type %u  number %u state %d \n", 
      //      event.value, event.type, event.number, buttons_state );
      
      // ignore the startup events
      if( event.type & JS_EVENT_INIT )
	{
	  //puts( "init" );
	  continue;
	}

      switch( event.type )
	{
	case JS_EVENT_BUTTON:
	  break;

	case JS_EVENT_AXIS:
	  {
	    //puts( "AXIS" );

	    switch( event.number )
	      {
		// SIDE-TO-SIDE
	      case XAXIS:
		//if( event.value > 0 ) 
		//puts( "turn right" );
		//else
		//puts( "turn left" );

		// set the robot turn rate
		cont->turnrate = NORMALIZE_TURN(-event.value);

		break;

		// FORWARD-AND-BACK
	      case YAXIS:
		//if( event.value > 0 ) 
		//puts( "forwards" );
		//else
		//puts( "backwards" );

		// set the robot velocity
		cont->speed = NORMALIZE_SPEED(-event.value);

		break;
		
		// TWISTING
	      case XAXIS2:
		//if( event.value > 0 ) 
		//puts( "twist left" );
		//else
		//puts( "twist right" );
		break;
		
		// THROTTLE
	      case YAXIS2:
		//if( event.value > 0 ) 
		//puts( "zoom out" );
		//else
		//puts( "zoom in" );
		cont->zoom = (int) KNORMALIZE(-event.value);
		break;

		// HAT UP DOWN
	      case XAXIS3:
		//if( event.value > 0 ) 
		//puts( "pan right" );
		//else
		//puts( "stop pan right" );
		
		if( event.value > 0 )
		  cont->pan = -PAN_SPEED;
		else
		  cont->pan = 0;
	
		break;
		
		// HAT LEFT RIGHT
	      case YAXIS3:
		//if( event.value > 0 ) 
		//puts( "pan left" );
		//else
		//puts( "stop pan left" );
	
		if( event.value > 0 )
		  cont->pan = +PAN_SPEED;
		else
		  cont->pan = 0;

		break;
	      }
	  }	  
	  break;
	}
      

    }
}

int main(int argc, char** argv)
{
  /* first, parse command line args */
  parse_args(argc,argv);

  /* Connect to the Player server */
  PlayerClient robot(host,port);

  PositionProxy pp(&robot,0,'a');
  PtzProxy ptzp(&robot,0,'a' );

  if(pp.GetAccess() == 'e') {
    puts("Error getting position device access!");
    exit(1);
  }

  // try a few reads
  for( int i=0; i<4; i++ )
    robot.Read();

  // get the initial values
  // now we can get the starting camera values 
  int pan = ptzp.pan;
  int tilt = ptzp.tilt;

  struct controller cont;
  memset( &cont, 0, sizeof(cont) );
  
  cont.speed = pp.speed;
  cont.turnrate = pp.turnrate;
 
  // kick off the joystick thread to generate new values in cont
  pthread_t dummy;
  pthread_create( &dummy, NULL,
                 (void *(*) (void *)) joystick_handler, &cont); 
  
  while( true )
    {
      robot.Read();
      
      printf( " speed: %d turn: %d pan: %d(%d)"
	      " tilt: %d(%d) zoom: %d              \r", 
	      cont.speed, cont.turnrate, 
	      pan, cont.pan, 
	      tilt, cont.tilt, 
	      cont.zoom );
      
      fflush( stdout );
      
      /* write commands to robot */
      pp.SetSpeed( cont.speed, cont.turnrate);
      
      // joystick hat switch enables panning/tilting rather than
      // giving an actual value, so we have a little controller to
      // increment and decrement the pan and tilt.
      pan += cont.pan;

      if( pan > 100 ) pan = 100;
      if( pan < -100 ) pan = -100;
      
      tilt += cont.tilt;

      if( tilt > 25 ) tilt = 25;
      if( tilt < -25 ) tilt = -25;

      ptzp.SetCam( pan, tilt, cont.zoom );
    }
  
  return(0);
}
    
