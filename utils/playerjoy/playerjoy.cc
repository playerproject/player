/*
 * playerjoy.cc - control position devices with a joystick
 *
 * Author: Richard Vaughan 
 * Created: 25 July 2002
 * 
 * $Id$
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <termios.h>
#define KEYBOARD_SUPPORT 1

#if HAVE_LINUX_JOYSTICK_H
  #include <linux/joystick.h>
  #define JOYSTICK_SUPPORT 1
#else
  #define JOYSTICK_SUPPORT 0
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
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
  "USAGE: playerjoy [options] <host:port> [<host:port>] ... \n" \
  "       -v   : verbose mode; print Player device state on stdout\n" \
  "       -3d  : connect to position3d interface (instead of position)\n" \
  "       -c   : continuously send commands\n" \
  "       -n   : dont send commands or enable motors (debugging)\n" \
  "       -k   : use keyboard control\n" \
  "       -p   : print out speeds on the console\n" \
  "       -udp : use UDP instead of TCP\n" \
  "       -speed     : maximum linear speed (m/sec)\n" \
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
#define KEYCODE_M 0x6d
#define KEYCODE_COMMA 0x2c
#define KEYCODE_PERIOD 0x2e

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

// do we print out speeds?
bool print_speeds = false;

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

public:
  Client(char* host, int port ); // constructor
  
  void Read( void ); // get data from Player
  void Update( struct controller* cont ); // send commands to Player
};

// type for a list of pointers to Client objects
typedef std::list<Client*> ClientList;

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
double max_speed = 0.500; // m/second
double max_turn = DTOR(60); // rad/second

// this is the speed that the camera will pan when you press the
// hatswitches in degrees/sec
/*
#define PAN_SPEED 2
#define TILT_SPEED 2
*/


// normalizing macros
#define AXIS_MAX            32767.0
#define PERCENT(X)          ((X) * 100.0 / AXIS_MAX)
#define PERCENT_POSITIVE(X) (((PERCENT(X)) + 100.0) / 2.0)
#define KNORMALIZE(X)       ((((X) * 1024.0 / AXIS_MAX)+1024.0) / 2.0)
#define NORMALIZE_SPEED(X)  ( (X) * max_speed / AXIS_MAX )
#define NORMALIZE_TURN(X)  ( (X) * max_turn / AXIS_MAX )

struct controller
{
  double speed, turnrate;
  bool dirty; // use this flag to determine when we need to send commands
};

#if JOYSTICK_SUPPORT
// open a joystick device and read from it, scaling the values and
// putting them into the controller struct
void*
joystick_handler(void* arg)
{
  // cast to a recognized type
  struct controller* cont = (struct controller*)arg;

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
      
    // ignore the startup events
    if( event.type & JS_EVENT_INIT )
    {
      continue;
    }

    switch( event.type )
    {
      case JS_EVENT_AXIS:
      {
        switch( event.number )
        {
          // SIDE-TO-SIDE
          case XAXIS:
            // set the robot turn rate
            cont->turnrate = NORMALIZE_TURN(-event.value);
            cont->dirty = true;
            break;

            // FORWARD-AND-BACK
          case YAXIS:
            // set the robot velocity
            cont->speed = NORMALIZE_SPEED(-event.value);
            cont->dirty = true;
            break;
        }
      }	  
      break;
    }
  }
  return(NULL);
}
#endif

#if KEYBOARD_SUPPORT
// read commands from the keyboard 
void*
keyboard_handler(void* arg)
{
  int kfd = 0;
  char c;
  double max_tv = max_speed;
  double max_rv = max_turn;
  struct termios cooked, raw;

  int speed=0;
  int turn=0;

  // cast to a recognized type
  struct controller* cont = (struct controller*)arg;

  // get the console in raw mode
  tcgetattr(kfd, &cooked);
  memcpy(&raw, &cooked, sizeof(struct termios));
  raw.c_lflag &=~ (ICANON | ECHO);
  raw.c_cc[VEOL] = 1;
  raw.c_cc[VEOF] = 2;
  tcsetattr(kfd, TCSANOW, &raw);
  
  puts("Reading from keyboard");
  puts("---------------------------");
  puts("Moving around:");
  puts("   u    i    o");
  puts("   j    k    l");
  puts("   m    ,    .");
  puts("");
  puts("q/z : increase/decrease max speeds by 10%");
  puts("w/x : increase/decrease only linear speed by 10%");
  puts("e/c : increase/decrease only angular speed by 10%");
  puts("anything else : stop");
  puts("---------------------------");

  for(;;)
  {
    // get the next event from the keyboard
    if(read(kfd, &c, 1) < 0)
    {
      perror("read():");
      exit(-1);
    }

    switch(c)
    {
      case KEYCODE_I:
        speed = 1;
        turn = 0;
        cont->dirty = true;
        break;
      case KEYCODE_K:
        speed = 0;
        turn = 0;
        cont->dirty = true;
        break;
      case KEYCODE_O:
        speed = 1;
        turn = -1;
        cont->dirty = true;
        break;
      case KEYCODE_J:
        speed = 0;
        turn = 1;
        cont->dirty = true;
        break;
      case KEYCODE_L:
        speed = 0;
        turn = -1;
        cont->dirty = true;
        break;
      case KEYCODE_U:
        turn = 1;
        speed = 1;
        cont->dirty = true;
        break;
      case KEYCODE_COMMA:
        turn = 0;
        speed = -1;
        cont->dirty = true;
        break;
      case KEYCODE_PERIOD:
        turn = 1;
        speed = -1;
        cont->dirty = true;
        break;
      case KEYCODE_M:
        turn = -1;
        speed = -1;
        cont->dirty = true;
        break;
      case KEYCODE_Q:
        max_tv += max_tv / 10.0;
        max_rv += max_rv / 10.0;
        if(always_command)
          cont->dirty = true;
        break;
      case KEYCODE_Z:
        max_tv -= max_tv / 10.0;
        max_rv -= max_rv / 10.0;
        if(always_command)
          cont->dirty = true;
        break;
      case KEYCODE_W:
        max_tv += max_tv / 10.0;
        if(always_command)
          cont->dirty = true;
        break;
      case KEYCODE_X:
        max_tv -= max_tv / 10.0;
        if(always_command)
          cont->dirty = true;
        break;
      case KEYCODE_E:
        max_rv += max_rv / 10.0;
        if(always_command)
          cont->dirty = true;
        break;
      case KEYCODE_C:
        max_rv -= max_rv / 10.0;
        if(always_command)
          cont->dirty = true;
        break;
      default:
        speed = 0;
        turn = 0;
        cont->dirty = true;
    }
    if (cont->dirty == true)
    {
      cont->speed = speed * max_tv;
      cont->turnrate = turn * max_rv;		
    }
  }
  return(NULL);
}
#endif
      
Client::Client(char* host, int port )
{
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
      PLAYER_ERROR("Read failed. Quitting.");
      exit( -1 );
    }
  }

  // turn on the motors
  if(!threed)
  {
    if(pp->SetMotorState(1))
      PLAYER_WARN("Failed to turn on motor power");
  }
  else
  {
    if(pp3->SetMotorState(1))
      PLAYER_WARN("Failed to turn on motor power");
  }
  
  gettimeofday(&lastcommand,NULL);

  puts("Success");
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
      if(print_speeds)
        printf("%5.3f %5.3f\n", cont->speed, RTOD(cont->turnrate));
      
      // send the speed commands
      if(!threed)
        pp->SetSpeed(cont->speed, cont->turnrate);
      else
        pp3->SetSpeed(cont->speed, cont->turnrate);
    }
    else
      printf("%5.3f %5.3f\n", cont->speed, RTOD(cont->turnrate));
    lastcommand = curr;
  }
  else if(use_keyboard && 
          (((curr.tv_sec + (curr.tv_usec / 1e6)) -
            (lastcommand.tv_sec + (lastcommand.tv_usec / 1e6))) > 
           COMMAND_TIMEOUT_SEC))
  {
    if(!stopped)
    {
      cont->speed = cont->turnrate = 0.0;
      if(!debug_mode)
      {
        if(print_speeds)
          printf("%5.3f %5.3f\n", cont->speed, RTOD(cont->turnrate));
        if(!threed)
          pp->SetSpeed(0,0);
        else
          pp3->SetSpeed(0,0);
      }
      else
        printf("%5.3f %5.3f\n", cont->speed, cont->turnrate);
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
    else if( strcmp( argv[i], "-p" ) == 0 )
      print_speeds = true;
    else if( strcmp( argv[i], "-k" ) == 0 )
      use_keyboard = true;
    else if( strcmp( argv[i], "-speed" ) == 0 )
    {
      if(i++ < argc)
        max_speed = atof(argv[i]);
      else
      {
        puts(USAGE);
        exit(-1);
      }
    }
    else if( strcmp( argv[i], "-turnspeed" ) == 0 )
    {
      if(i++ < argc)
        max_turn = DTOR(atof(argv[i]));
      else
      {
        puts(USAGE);
        exit(-1);
      }
    }
    else if( strcmp( argv[i], "-udp" ) == 0 )
      protocol = PLAYER_TRANSPORT_UDP;
    else
    {
      puts(USAGE); // malformed arg - print usage hints
      exit(-1);
    }
  }

  // if no Players were requested, we assume localhost:6665
  if( clients.begin() == clients.end() )
    clients.push_front( new Client( DEFAULT_HOST, DEFAULT_PORT ));

  // this structure is maintained by the joystick reader
  struct controller cont;
  memset( &cont, 0, sizeof(cont) );
  
  pthread_t dummy;

  if(!use_keyboard)
  {
#if JOYSTICK_SUPPORT
    jfd = open ("/dev/js0", O_RDONLY);

    if(jfd < 1)
    {
      perror("Failed to open joystick");
      PLAYER_WARN("Falling back on keyboard control");
      use_keyboard = true;
    }
    else
      pthread_create(&dummy, NULL, &joystick_handler, (void*)&cont); 
#else
    PLAYER_WARN("Joystick support not included; falling back on keyboard control");
    use_keyboard = true;
#endif
  }

  if(use_keyboard)
  {
#if KEYBOARD_SUPPORT
    pthread_create(&dummy, NULL, &keyboard_handler, (void*)&cont); 
#else
    PLAYER_ERROR("Keyboard support not include; bailing.");
    exit(-1);
#endif
  }

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

