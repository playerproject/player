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
#include <termio.h>
#include <sys/ioctl.h>
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
  "USAGE: joystick [options] <host:port> [<host:port>] ... \n" \
  "       -v   : verbose mode; print Player device state on stdout\n" \
  "       -3d  : connect to position3d interface (instead of position)\n" \
  "       -c   : continuously send commands\n" \
  "       -n   : dont send commands or enable motors (debugging)\n" \
  "       -k   : use keyboard control\n" \
  "       -udp : use UDP instead of TCP\n" \
  "       -speed     : maximum linear speed (mm/sec)\n" \
  "       -turnspeed : maximum angular speed (deg/sec)\n" \
  "       <host:port> : connect to a Player on this host and port\n"


#define DEFAULT_HOST "localhost"
#define DEFAULT_PORT PLAYER_PORTNUM

#define KEYCODE_I 0x69
#define KEYCODE_J 0x6a
#define KEYCODE_K 0x6b
#define KEYCODE_L 0x6c
#define KEYCODE_Q 0x71
#define KEYCODE_Z 0x7a
#define KEYCODE_W 0x77
#define KEYCODE_X 0x78
#define KEYCODE_E 0x65
#define KEYCODE_C 0x63
#define KEYCODE_U 0x75
#define KEYCODE_O 0x6F

#define COMMAND_TIMEOUT_SEC 0.2

int idx = 0;

// flag to control the level of output - the -v arg sets this
int g_verbose = false;

// are we talking in 3D?
bool threed = false;

// should we continuously send commands?
bool always_command = false;

// are we in debug mode (dont send commands)?
bool debug_mode = false;

// use the keyboard instead of the joystick?
bool use_keyboard = false;

// joystick fd
int jfd;

// allow either TCP or UDP
int protocol = PLAYER_TRANSPORT_TCP;

// initial max speeds

// define a class to do interaction with Player
class Client
{
private:
  // these are the proxies we create to access the devices
  PlayerClient *player;
  PositionProxy *pp;
  Position3DProxy *pp3;

  struct timeval lastcommand;

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
int max_speed = 500; // mm/second
int max_turn = 60; // degrees/second

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
#define NORMALIZE_SPEED(X)  ( X * max_speed / AXIS_MAX )
#define NORMALIZE_TURN(X)  ( X * max_turn / AXIS_MAX )

struct controller
{
  int speed, turnrate; //pan, tilt, zoom;
  bool dirty; // use this flag to determine when we need to send commands
};

// open a joystick device and read from it, scaling the values and
// putting them into the controller struct
void joystick_handler(struct controller* cont)
{
  // cast to a recognized type
  //struct controller* cont = (struct controller*)arg;

  struct js_event event;
  
  int buttons_state = 0;

  //puts("Reading from joystick");

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

// read commands from the keyboard 
void keyboard_handler(struct controller* cont )
{
  int kfd = 0;
  char c;
  double max_tv = 500.0;
  double max_rv = 10.0;
  struct termio cooked, raw;

	int speed,turn;
	speed = turn = 0;

  // get the console in raw mode
  ioctl(kfd, TCGETA, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termio));
  raw.c_lflag &=~ (ICANON | ECHO);
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  ioctl(kfd, TCSETA, &raw);
  
  puts("Reading from keyboard");
  puts("---------------------------");
  puts("Moving around:");
  puts("i : forward");
  puts("j : left");
  puts("l : right");
  puts("k : backward");
  puts("u : stop turn");
  puts("o : stop movement (but still turn)");
  puts("q/z : increase/decrease speed by 10%");
  puts("w/x : increase/decrease only pos speed by 10%");
  puts("e/c : increase/decrease only turn speed by 10%");
  puts("anything else : stop");
  puts("---------------------------");

  for(;;)
  {
    // get the next event from the joystick
    if(read(kfd, &c, 1) < 0)
    {
      perror("read():");
      exit(-1);
    }

    switch(c)
    {
      case KEYCODE_I:
		speed = 1;
        cont->dirty = true;
        break;
      case KEYCODE_K:
		speed = -1;
        cont->dirty = true;
        break;
      case KEYCODE_O:
		speed = 0;
        cont->dirty = true;
        break;
      case KEYCODE_J:
	  	turn = 1;
        cont->dirty = true;
        break;
      case KEYCODE_L:
	  	turn = -1;
        cont->dirty = true;
        break;
      case KEYCODE_U:
	  	turn = 0;
        cont->dirty = true;
        break;
      case KEYCODE_Q:
        cont->dirty = true;
        max_tv += max_tv / 10.0;
        max_rv += max_rv / 10.0;
        break;
      case KEYCODE_Z:
        cont->dirty = true;
        max_tv -= max_tv / 10.0;
        max_rv -= max_rv / 10.0;
		// if we get to 0 then increasing by 10% is meaningless
		// so lets not go below 1
        if(max_tv < 1)
          max_tv = 1;
        if(max_rv < 1)
          max_rv = 1;
        break;
      case KEYCODE_W:
        cont->dirty = true;
        max_tv += max_tv / 10.0;
        break;
      case KEYCODE_X:
        cont->dirty = true;
        max_tv -= max_tv / 10.0;
        if(max_tv < 1)
          max_tv = 1;
        break;
      case KEYCODE_E:
        cont->dirty = true;
        max_rv += max_rv / 10.0;
        break;
      case KEYCODE_C:
        cont->dirty = true;
        max_rv -= max_rv / 10.0;
        if(max_rv < 1)
          max_rv = 1;
        break;
      default:
	  	speed = 0;
		turn = 0;
        cont->dirty = true;
		
    }
	if (cont->dirty == true)
	{
        cont->speed = speed * (int)max_tv;
        cont->turnrate = turn * (int)max_rv;		
	}

  }
}
      
Client::Client(char* host, int port )
{
  //if( g_verbose )
    printf( "Connecting to Player at %s:%d - ", host, port );
    fflush( stdout );
  
  /* Connect to the Player server */
  assert( player = new PlayerClient(host,port,protocol) );  
  if(!threed)
  {
    assert( pp = new PositionProxy(player,idx,'a') );
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
      puts("WARNING: failed to turn on motor power");
  }
  else
  {
    if(pp3->SetMotorState(1))
      puts("WARNING: failed to turn on motor power");
  }
  
  // store a local copy of the initial p&t
  //pan = ptzp->pan;
  //tilt = ptzp->tilt;

  gettimeofday(&lastcommand,NULL);

  puts( "Success" );
}

void Client::Read( void )
{
  if(player->Read())
    exit(-1);
}

void Client::Update( struct controller* cont )
{
  struct timeval curr;
  static bool stopped = false;

  if(g_verbose)
  {
    if(!threed)
      pp->Print();
    else
      pp3->Print();
  }

  gettimeofday(&curr,NULL);
  
  if(cont->dirty || always_command) // if the joystick sent a command
  {
    stopped = false;
    if(!debug_mode)
    {
      // send the speed commands
      if(!threed)
        pp->SetSpeed( cont->speed, cont->turnrate);
      else
        pp3->SetSpeed( cont->speed, cont->turnrate);
    }
    else
      printf("%d %d\n", cont->speed, cont->turnrate);
    lastcommand = curr;
  }
  else if(((curr.tv_sec + (curr.tv_usec / 1e6)) -
           (lastcommand.tv_sec + (lastcommand.tv_usec / 1e6))) > 
          COMMAND_TIMEOUT_SEC)
  {
    if(!stopped)
    {
      cont->speed = cont->turnrate = 0;
      if(!debug_mode)
      {
        if(!threed)
          pp->SetSpeed(0,0);
        else
          pp3->SetSpeed(0,0);
      }
      else
	printf("%d %d\n", cont->speed, cont->turnrate);
      stopped = true;
    }
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
      else if( strcmp( argv[i], "-i" ) == 0 )
      {
        if(i++ < argc)
          idx = atoi(argv[i]);
        else
        {
          puts(USAGE);
          exit(-1);
        }
      }
      else if( strcmp( argv[i], "-v" ) == 0 )
        g_verbose = true;
      else if( strcmp( argv[i], "-3d" ) == 0 )
        threed = true;
      else if( strcmp( argv[i], "-c" ) == 0 )
        always_command = true;
      else if( strcmp( argv[i], "-n" ) == 0 )
        debug_mode = true;
      else if( strcmp( argv[i], "-k" ) == 0 )
        use_keyboard = true;
      else if( strcmp( argv[i], "-speed" ) == 0 )
      {
        if(i++ < argc)
          max_speed = atoi(argv[i]);
        else
        {
          puts(USAGE);
          exit(-1);
        }
      }
      else if( strcmp( argv[i], "-turnspeed" ) == 0 )
      {
        if(i++ < argc)
          max_turn = atoi(argv[i]);
        else
        {
          puts(USAGE);
          exit(-1);
        }
      }
      else if( strcmp( argv[i], "-udp" ) == 0 )
        protocol = PLAYER_TRANSPORT_UDP;
      else
        puts( USAGE ); // malformed arg - print usage hints
    }

  // if no Players were requested, we assume localhost:6665
  if( clients.begin() == clients.end() )
    clients.push_front( new Client( DEFAULT_HOST, DEFAULT_PORT ));
  
    // this structure is maintained by the joystick reader
  struct controller cont;
  memset( &cont, 0, sizeof(cont) );

  if(!use_keyboard)
  {
    jfd = open ("/dev/js0", O_RDONLY);

    if( jfd < 1 )
    {
      perror( "Failed to open joystick" );
      use_keyboard = true;
    }
  }

  // kick off the thread to generate new values in cont
  pthread_t dummy;
  if(use_keyboard)
    pthread_create( &dummy, NULL,
                    (void *(*) (void *)) keyboard_handler, &cont); 
  else
    pthread_create( &dummy, NULL,
                    (void *(*) (void *)) joystick_handler, &cont); 
  
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
    
