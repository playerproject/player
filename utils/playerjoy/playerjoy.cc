/*
 * playerjoy.cc - control position devices with a joystick
 *
 * Author: Richard Vaughan 
 * Created: 25 July 2002
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

#include <list>

#define USAGE \
  "USAGE: joystick [-v] <host:port> [<host:port>] ... \n" \
  "       -v : verbose mode; print Player device state on stdout\n" \
  "       <host:port> : connect to a Player on this host and port\n"


#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT PLAYER_PORTNUM

// flag to control the level of output - the -v arg sets this
int g_verbose = false;

// are we talking in 3D?
bool threed = false;

// define a class to do interaction with Player
class Client
{
private:
  // these are the proxies we create to access the devices
  PlayerClient *player;
  PositionProxy *pp;
  Position3DProxy *pp3;
  /*
  PtzProxy *ptzp;

  int pan, tilt;
  */

public:
  Client(char* host, int port ); // constructor
  
  void Read( void ); // get data from Player
  void Update( struct controller* cont ); // send commands to Player
};

// type for a list of pointers to Client objects
typedef std::list<Client*> ClientList;

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
#define PAN    4
#define TILT   5
#define XAXIS4 6
#define YAXIS4 7

// define the speed limits for the robot

// at full joystick depression you'll go this fast
#define MAX_SPEED    2000 // mm/second
#define MAX_TURN    60 // degrees/second

// this is the speed that the camera will pan when you press the
// hatswitches in degrees/sec
/*
#define PAN_SPEED 2
#define TILT_SPEED 2
*/


// normalizing macros
#define AXIS_MAX            32767.0
#define PERCENT(X)          (X * 100.0 / AXIS_MAX)
#define PERCENT_POSITIVE(X) (((PERCENT(X)) + 100.0) / 2.0)
#define KNORMALIZE(X)       (((X * 1024.0 / AXIS_MAX)+1024.0) / 2.0)
#define NORMALIZE_SPEED(X)  ( X * MAX_SPEED / AXIS_MAX )
#define NORMALIZE_TURN(X)  ( X * MAX_TURN / AXIS_MAX )

struct controller
{
  int speed, turnrate; //pan, tilt, zoom;
  bool dirty; // use this flag to determine when we need to send commands
};

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

    //puts( "JS" );
      
    if ((event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON) {
      if (event.value)
        buttons_state |= (1 << event.number);
      else
        buttons_state &= ~(1 << event.number);
    }
      
    /*
      printf( "value % d type %u  number %u state %d \n", 
      event.value, event.type, event.number, buttons_state );
    */
      
    // ignore the startup events
    if( event.type & JS_EVENT_INIT )
    {
      //puts( "init" );
      continue;
    }

    switch( event.type )
    {
      /*
      case JS_EVENT_BUTTON:
        if(event.number == 2 && buttons_state)
          cont->pan += PAN_SPEED;
        else if(event.number == 3 && buttons_state)
          cont->pan -= PAN_SPEED;
        else if(event.number == 2 || event.number == 3)
          cont->pan = 0;
        break;
        */
        
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
            cont->turnrate = (int)NORMALIZE_TURN(-event.value);
            cont->dirty = true;
            break;

            // FORWARD-AND-BACK
          case YAXIS:
            //if( event.value > 0 ) 
            //puts( "forwards" );
            //else
            //puts( "backwards" );

            // set the robot velocity
            cont->speed = (int)NORMALIZE_SPEED(-event.value);
            cont->dirty = true;

            break;
		
            /*
            // TWISTING
          case XAXIS2:
          case YAXIS2:
            //if( event.value > 0 ) 
            //puts( "zoom out" );
            //else
            //puts( "zoom in" );
            cont->zoom = (int)KNORMALIZE(-event.value);
            cont->dirty = true;

            break;
            */

            /*
              // HAT UP DOWN
              case TILT:
              if( event.value > 0 ) 
              puts( "tilt down" );
              else if( event.value < 0 ) 
              puts( "tilt up" );
              else
              puts( "stop tilt" );
		
              if( event.value > 0 )
              cont->tilt = -TILT_SPEED;
              else if( event.value < 0 )
              cont->tilt = TILT_SPEED;
              else
              cont->tilt = 0;
	
              //cont->dirty = true;
	
              break;
		
              // HAT LEFT RIGHT
              case PAN:
              if( event.value > 0 ) 
              puts( "pan right" );
              else if( event.value < 0 ) 
              puts( "pan left" );
              else
              puts( "stop pan" );
	
              if( event.value > 0 )
              cont->pan = -PAN_SPEED;
              else if( event.value < 0 )
              cont->pan = PAN_SPEED;
              else
              cont->pan = 0;
		
              //cont->dirty = true;
	
              break;
            */
	      }
      }	  
      break;
    }
      

  }
}
      
Client::Client(char* host, int port )
{
  //if( g_verbose )
    printf( "Connecting to Player at %s:%d - ", host, port );
    fflush( stdout );
  
  /* Connect to the Player server */
  assert( player = new PlayerClient(host,port) );  
  if(!threed)
  {
    assert( pp = new PositionProxy(player,0,'a') );
    if(pp->GetAccess() == 'e') 
    {
      puts("Error getting position device access!");
      exit(1);
    }
  }
  else
  {
    assert( pp3 = new Position3DProxy(player,0,'a') );
    if(pp3->GetAccess() == 'e') 
    {
      puts("Error getting position3d device access!");
      exit(1);
    }
  }
  
  // try a few reads
  for( int i=0; i<4; i++ )
  {
    if(player->Read() < 0 )
    {
      puts( " - Failed. Quitting." );
      exit( -1 );
    }
  }

  // turn on the motors
  if(!threed)
  {
    if(pp->SetMotorState(1))
      exit(-1);
  }
  else
  {
    if(pp3->SetMotorState(1))
      exit(-1);
  }
  
  // store a local copy of the initial p&t
  //pan = ptzp->pan;
  //tilt = ptzp->tilt;

  puts( "Success" );
}

void Client::Read( void )
{
  if(player->Read())
    exit(-1);
}

void Client::Update( struct controller* cont )
{
  if(g_verbose)
  {
    if(!threed)
      pp->Print();
    else
      pp3->Print();
  }
  
  if( cont->dirty ) // if the joystick sent a command
  {
    // send the speed commands
    if(!threed)
      pp->SetSpeed( cont->speed, cont->turnrate);
    else
      pp3->SetSpeed( cont->speed, cont->turnrate);
  }
}


int main(int argc, char** argv)
{

  ClientList clients;

  // parse command line args to construct clients
  for( int i=1; i<argc; i++ )
    {
      // if we find a colon in the arg, it's a player address
      if( char* colon = index( argv[i], ':'  ) )
	{
	  // replace the colon with a terminator
	  *colon = 0; 	  
	  // now argv[i] is a hostname string
	  // and colon=1 is a port number string	  
	  clients.push_front( new Client( argv[i], atoi( colon+1 ) ));
	}
      // otherwise look for the verbose flag
      else if( strcmp( argv[i], "-v" ) == 0 )
	g_verbose = true;
      else if( strcmp( argv[i], "-3d" ) == 0 )
	threed = true;
      else
	puts( USAGE ); // malformed arg - print usage hints
    }

  // if no Players were requested, we assume localhost:6665
  if( clients.begin() == clients.end() )
    clients.push_front( new Client( DEFAULT_HOST, DEFAULT_PORT ));
  
    // this structure is maintained by the joystick reader
  struct controller cont;
  memset( &cont, 0, sizeof(cont) );
  
  // kick off the joystick thread to generate new values in cont
  pthread_t dummy;
  pthread_create( &dummy, NULL,
                 (void *(*) (void *)) joystick_handler, &cont); 
  
  puts( "Reading joystick" );

  while( true )
    {
      // read from all the clients
      for( ClientList::iterator it = clients.begin();
	   it != clients.end();
	   it++ )
	(*it)->Read();
      
      // update all the clients
      for( ClientList::iterator it = clients.begin();
	   it != clients.end();
	   it++ )
	(*it)->Update( &cont );
     

      cont.dirty = false; // we've handled the changes
    }
  return(0);
}
    
