/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * the main code for the Player robot server.  here we instantiate
 * the device objects, do socket connection control and spawn
 * client reader/writer threads.
 */

//#define VERBOSE
//#define DEBUG

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if HAVE_SYS_TYPES_H
  #include <sys/types.h>  /* for accept(2) */
#endif

#include <dirent.h>

#if HAVE_LIB_DL
#include <dlfcn.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h> // for bzero()
#include <stdlib.h>  /* free(3),exit(3) */
#include <signal.h>
#include <netinet/in.h> /* for struct sockaddr_in, SOCK_STREAM */
#include <unistd.h>  /* for close(2) */
#include <sys/socket.h>  /* for accept(2) */
#include <netdb.h> /* for gethostbyaddr(3) */

#include <socket_util.h> /* for create_and_bind_socket() */
#include <deviceregistry.h> /* for register_devices() */
#include <configfile.h> /* for config file parser */

#include <clientdata.h>
#include <clientmanager.h>
#include <devicetable.h>
#include <drivertable.h>

// the base class and two derived classes for timefeeds:
#include <playertime.h>
#include <wallclocktime.h>

#include <sys/mman.h> // for mmap
#include <fcntl.h>

#include "replace.h"

#include <stagetime.h>
#include <stagedevice.h>
player_stage_info_t *arenaIO; //address for memory mapped IO to Stage
char stage_io_directory[MAX_FILENAME_SIZE]; // filename for mapped memory

// true if we're connecting to Stage instead of a real robot
bool use_stage = false;

// enable "special" extensions
bool player_gerkey = false;

size_t ioSize = 0; // size of the IO buffer


// this table holds all the currently *instantiated* devices
CDeviceTable* deviceTable = new CDeviceTable();
// this table holds all the currently *available* drivers
DriverTable* driverTable = new DriverTable();

// the global PlayerTime object has a method 
//   int GetTime(struct timeval*)
// which everyone must use to get the current time
PlayerTime* GlobalTime;

// keep track of the pointers to our various clients.
// that way we can cancel them at Shutdown
ClientManager* clientmanager;

// use this object to parse config files and command line args
ConfigFile configFile;

// for use in other places (cliendata.cc, for example)
char playerversion[] = VERSION;

bool experimental = false;
bool debug = false;

int global_playerport = PLAYER_PORTNUM; // used to gen. useful output & debug

extern player_interface_t interfaces[];

/* Usage statement */
void
Usage()
{
  puts("");
  fprintf(stderr, "USAGE: player [-p <port>] [-s <path>] [-dl <shlib>] "
          "[-k <key>] [<configfile>]\n");
  fprintf(stderr, "  -p <port>     : TCP port where Player will listen. "
          "Default: %d\n", PLAYER_PORTNUM);
  fprintf(stderr, "  -s <path>     : use memory-mapped IO with Stage "
          "through the devices in\n                  this directory\n");
  fprintf(stderr, "  -d <shlib>    : load the the indicated shared library\n");
  fprintf(stderr, "  -k <key>      : require client authentication with the "
          "given key\n");
  fprintf(stderr, "  <configfile>  : load the the indicated config file\n");
}

/* just so we know when we've segfaulted, even when running under stage */
void 
printout_segv( int dummy ) 
{
  puts("Player got a SIGSEGV! (that ain't good...)");
  exit(-1);
}
/* just so we know when we've bus errored, even when running under stage */
void 
printout_bus( int dummy ) 
{
  puts("Player got a SIGBUS! (that ain't good...)");
  exit(-1);
}

/* sighandler to shut everything down properly */
void 
Interrupt( int dummy ) 
{
  if(dummy != SIGTERM)
  {
    // delete the manager; it will delete the clients
    delete clientmanager;

    // next, delete the deviceTable; it will delete all the devices internally
    delete deviceTable;
  }

  if(use_stage)
    puts("** Player quitting **" );
  else
    printf("** Player [port %d] quitting **\n", global_playerport );
  
  exit(0);
}

/* setup some signal handlers */
void 
SetupSignalHandlers()
{
  if(signal(SIGSEGV, printout_segv) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGSEGV");
    exit(1);
  }
  if(signal(SIGBUS, printout_bus) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGBUS");
    exit(1);
  }

  /* set up to handle SIGPIPE (happens when the client dies) */
  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGPIPE");
    exit(1);
  }

  if(signal(SIGINT, Interrupt ) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGINT");
    exit(1);
  }

  if(signal(SIGHUP, Interrupt ) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGHUP");
    exit(1);
  }

  if(signal(SIGTERM, Interrupt) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGTERM");
    exit(1);
  }
}


/* for debugging */
void PrintHeader(player_msghdr_t hdr)
{
  printf("stx: %u\n", hdr.stx);
  printf("type: %u\n", hdr.type);
  printf("device: %u\n", hdr.device);
  printf("index: %u\n", hdr.device_index);
  printf("time: %u:%u\n", hdr.time_sec,hdr.time_usec);
  printf("times: %u:%u\n", hdr.timestamp_sec,hdr.timestamp_usec);
  printf("reserved: %u\n", hdr.reserved);
  printf("size:%u\n", hdr.size);
}


// a simple function to conditionally add a port (if is new) to 
// a list of ports
void 
StageAddPort(int* ports, int* portcount, int newport)
{
  // have we registered this port already?
  int j = 0;
  while(j<(*portcount))
  {
    if(ports[j] == newport)
      break;
    j++;
  }
  if(j == (*portcount)) // we have not! 
  {
    // we add this port to the array of ports we must listen on
    ports[(*portcount)++] = newport;
  }
}

// a matching function to indentify valid device names
// used by scandir to fetch device filenames
int
MatchDeviceName( const struct dirent* ent )
{
  // device names are > 2 chars long,; . and .. are not
  return( strlen( ent->d_name ) > 2 );
}

// looks in the directory for device entries, creates the devices
// and fills an array with unique port numbers
stage_clock_t* 
CreateStageDevices( char* directory, int** ports, int* num_ports )
{
#ifdef VERBOSE
  printf( "Searching for Stage devices\n" );
#endif

  struct dirent **namelist;
  int n = -1;
  
  int tfd = 0;
  
  char devicefile[ MAX_FILENAME_SIZE ];

  int portcount = 0;
  
  int* portstmp = new int[MAXPORTS];


  // open the lock file
  char lockfile[ MAX_FILENAME_SIZE ];
  sprintf( lockfile, "%s/%s", directory, STAGE_LOCK_NAME );

  int lockfd = -1;

  if( (lockfd = open( lockfile, O_RDWR )) < 1 )
  {
    printf( "PLAYER: opening lock file %s (%d)\n", lockfile, lockfd );
    perror("Failed to create lock device" );
    return NULL;
  } 
 
  // open all the files in the IO directory
  n = scandir( directory, &namelist, MatchDeviceName, 0);
  if (n < 0)
    perror("scandir");
  else if(!n)
    PLAYER_WARN1("found no stage device files in directory \"%s\"", directory);
  else
  {
    // for every file in the directory, create a new device
    while(n--)
    {
      // don't try to load the clock here - we'll do it below
      if( strcmp( STAGE_CLOCK_NAME, namelist[n]->d_name ) == 0 )
        continue;
      
      // don't try to open the lock here - we already did it above
      if( strcmp( STAGE_LOCK_NAME, namelist[n]->d_name ) == 0 )
        continue;

#ifdef DEBUG      
      printf("Opening %s ", namelist[n]->d_name);
      fflush( stdout );
#endif
      
      sprintf( devicefile, "%s/%s", directory, namelist[n]->d_name ); 
      
      if( (tfd = open(devicefile, O_RDWR )) < 0 )
      {
        perror( "Failed to open IO file" );
        printf("Tried to open file \"%s\"\n", devicefile);
        exit( -1 );
      }
    
      // find out how big the file is - we need to mmap that many bytes
      int ioSize = lseek( tfd, 0, SEEK_END );

#ifdef VERBOSE
      printf( "Mapping %d bytes.\n", ioSize );
      fflush( stdout );
#endif
      
      player_stage_info_t* deviceIO = 0;
      
      
      if((deviceIO = 
          (player_stage_info_t*)mmap( NULL, ioSize, PROT_READ | PROT_WRITE, 
                                      MAP_SHARED, tfd, (off_t)0 ))  
         == MAP_FAILED )
      {
        perror( "Failed to map memory" );
        exit( -1 );
      }
      
      close( tfd ); // can close fd once mapped
      
      StageDevice *dev = 0; // declare outside switch statement

      // prime the configFile parser
      int globalparent = configFile.AddEntity(-1,"");

      // get the player type and index from the header
      // NOT from the filename
      switch(deviceIO->player_id.code)
      {
        // create a generic simulated stage IO device for these types:
        case PLAYER_PLAYER_CODE: 
        case PLAYER_POSITION_CODE:
        case PLAYER_LASER_CODE:
        case PLAYER_SONAR_CODE:
        case PLAYER_BLOBFINDER_CODE:  
        case PLAYER_PTZ_CODE:     
        case PLAYER_FIDUCIAL_CODE: 
        case PLAYER_TRUTH_CODE:
        case PLAYER_GPS_CODE:
        case PLAYER_GRIPPER_CODE:
        case PLAYER_IDAR_CODE:
        case PLAYER_IDARTURRET_CODE:
        case PLAYER_DESCARTES_CODE:
        case PLAYER_MOTE_CODE:
        {
          // Create a StageDevice with this IO base address and filedes
          dev = new StageDevice( deviceIO, lockfd, deviceIO->lockbyte );
	  
	  deviceTable->AddDevice(deviceIO->player_id, 
                                 (char*)(deviceIO->drivername),
                                 PLAYER_ALL_MODE, dev);
	  
          // add this port to our listening list
          StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
        }
        break;
	  
        case PLAYER_COMMS_CODE:   
          // Create broadcast device as per normal
          //
          // FIXME: apparently deviceIO->local is *not* currently being set
          // by Stage...
          if(deviceIO->local || !deviceIO->local)
          {
            // Broadcast through the loopback device; note that this
            // wont work with distributed Stage.

            // the following code is required to load settings into the
            // ConfigFile object.  should be cleaned up.
            int section = configFile.AddEntity(globalparent,"broadcast");
            int value = configFile.AddProperty(section, "addr", 0);
            configFile.AddPropertyValue(value, 0, 0);
            configFile.AddToken(0, "", 0);
            configFile.WriteString(section, "addr", "127.255.255.255");

            // find the broadcast device in the available device table
            DriverEntry* entry;
            if(!(entry = driverTable->GetDriverEntry("udpbroadcast")))
            {
              puts("WARNING: Player support for broadcast device unavailable.");
            }
            else
            {
              // add it to the instantiated device table
              deviceTable->AddDevice(deviceIO->player_id, "broadcast",
                                     PLAYER_ALL_MODE, 
                                     (*(entry->initfunc))(PLAYER_COMMS_STRING,
                                                   &configFile, section));
 
              // add this port to our listening list
              StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
            }
          }
          break;

          /*
        case PLAYER_BPS_CODE:   
          // Create broadcast device as per normal
          
          // FIXME: apparently deviceIO->local is *not* currently being set
          // by Stage...
          if(deviceIO->local || !deviceIO->local)
          {
            // find the bps device in the available device table
            DriverEntry* entry;
            if(!(entry = driverTable->GetDriverEntry("bps")))
            {
              puts("WARNING: Player support for bps device unavailable.");
            }
            else
            {
              // add it to the instantiated device table
              deviceTable->AddDevice(deviceIO->player_id, PLAYER_ALL_MODE, 
                           (*(entry->initfunc))(PLAYER_BPS_STRING,
                                                &configFile,0));
 
              // add this port to our listening list
              StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
            }
          }
          break;
          */

          // devices not implemented
        case PLAYER_AUDIO_CODE:   
        case PLAYER_POWER_CODE:
        case PLAYER_AIO_CODE:
        case PLAYER_DIO_CODE:
        case PLAYER_BUMPER_CODE:
          printf("Device type %d not yet implemented in Stage\n", 
                 deviceIO->player_id.code);
          break;
	  
        case 0:
#ifdef VERBOSE
          printf("Player ignoring Stage device type %d\n", 
                 deviceIO->player_id.code);
#endif
          break;	  
	  
          // unknown device 
        default:
          printf( "Unknown device type %d for object ID (%d,%d,%d)\n",
                  deviceIO->player_id.code, 
                  deviceIO->player_id.port, 
                  deviceIO->player_id.code, 
                  deviceIO->player_id.index ); 
          break;
      }
      free(namelist[n]);
    }
    free(namelist);
  }
  

  // we've discovered all thew ports now, so allocate memory for the
  // required port numbers at the return pointer
  assert( *ports = new int[ portcount ] );
  
  // copy the port numbers in
  memcpy( *ports, portstmp, portcount * sizeof(int) );
  
  // set the number of ports detected
  *num_ports = portcount;

  // clean up
  delete[] portstmp;

  // open and map the stage clock
  char clockname[MAX_FILENAME_SIZE];
  snprintf( clockname, MAX_FILENAME_SIZE-1, "%s/%s", 
	    directory, STAGE_CLOCK_NAME );
  
#ifdef DEBUG
  printf("Opening %s\n", clockname );
#endif
  
  if( (tfd = open( clockname, O_RDWR )) < 0 )
  {
    perror( "Failed to open clock file" );
    printf("Tried to open file \"%s\"\n", clockname );
    exit( -1 );
  }
  
    
  //struct timeval * stage_tv = 0;    
 
  stage_clock_t *clock = 0;

  if( (clock = (stage_clock_t*)mmap( NULL, sizeof(struct timeval),
				     PROT_READ | PROT_WRITE, 
                                     MAP_SHARED, tfd, (off_t)0 ) )
      == MAP_FAILED )
  {
    perror( "Failed to map clock memory" );
    exit( -1 );
  }
  
  //close( tfd ); // can close fd once mapped

  
#ifdef DEBUG  
  printf( "finished creating stage devices\n" );
  fflush( stdout );
#endif
  
  // set up the stagetime object
  assert( clock );
  
  // GlobalTime is initialized in Main(), so we'll delete and reassign it here
  if(GlobalTime)
    delete GlobalTime;
  assert(GlobalTime = (PlayerTime*)(new StageTime(clock, tfd)));

  return( clock );
}

bool
parse_config_file(char* fname)
{
  DriverEntry* entry;
  CDevice* tmpdevice;
  char interface[PLAYER_MAX_DEVICE_STRING_LEN];
  char* colon;
  int index;
  char* driver;
  int code = 0;
  
  // parse the file
  printf("\n\nParsing configuration file \"%s\"...\n", fname);

  if(!configFile.Load(fname))
    return(false);

  // load each device specified in the file
  for(int i = 1; i < configFile.GetEntityCount(); i++)
  {
    if(configFile.entities[i].type < 0)
      continue;

    // get the interface name
    strncpy(interface, (const char*)(configFile.entities[i].type), 
            sizeof(interface));
    interface[sizeof(interface)-1] = '\0';

    // look for a colon and trailing index
    if((colon = strchr(interface,':')) && strlen(colon) >= 2)
    {
      // get the index out
      index = atoi(colon+1);
      // strip the index off the end
      interface[colon-interface] = '\0';
    }
    else
      index = 0;

    driver = NULL;
    // find the default driver for this interface
    player_interface_t tmpint;
    if(lookup_interface(interface, &tmpint) == 0)
    {
      driver = tmpint.default_driver;
      code = tmpint.code;
    }
    if(!driver)
    {
      PLAYER_ERROR1("Couldn't find interface \"%s\"", interface);
      exit(-1);
    }

    // did the user specify a different driver?
    driver = (char*)configFile.ReadString(i, "driver", driver);

    printf("  loading driver \"%s\" as device \"%s:%d\"\n", 
           driver, interface, index);
    /* look for the indicated driver in the available device table */
    if(!(entry = driverTable->GetDriverEntry(driver)))
    {
      PLAYER_ERROR1("Couldn't find driver \"%s\"", driver);
      return(false);
    }
    else
    {
      player_device_id_t id;
      id.code = code;
      id.port = global_playerport;
      id.index = index;

      if(!(tmpdevice = (*(entry->initfunc))(interface,&configFile,i)))
      {
        PLAYER_ERROR2("Initialization failed for driver \"%s\" as interface \"%s\"\n",
                      driver, interface);
        exit(-1);
      }
      deviceTable->AddDevice(id, driver, entry->access, tmpdevice);

      // should this device be "always on"?
      if(configFile.ReadInt(i, "alwayson", 0))
      {
        if(tmpdevice->Subscribe(NULL))
        {
          PLAYER_ERROR2("Initial subscription failed to driver \"%s\" as interface \"%s\"\n",
                        driver,interface);
          exit(-1);
        }
      }
    }
  }

  configFile.WarnUnused();

  puts("Done.");

  return(true);
}

int main( int argc, char *argv[] )
{
  struct sockaddr_in listener;
  char auth_key[PLAYER_KEYLEN] = "";

  // setup the clock;  if we end up using Stage, we'll delete this one
  // and reassign it
  assert(GlobalTime = (PlayerTime*)(new WallclockTime()));

  // Register the available drivers in the driverTable.  
  //
  // Although most of these devices are only used with physical devices, 
  // some (e.g., broadcast, bps) are used with Stage, and so we always call 
  // this function.
  register_devices();
  
  // use these to keep track of many sockets in stage mode
  struct pollfd* ufds = 0;
  int* ports = 0;
  int num_ufds = 0;

  SetupSignalHandlers();

  printf("** Player v%s ** ", playerversion);
  fflush(stdout);

  // parse args
  for( int i = 1; i < argc; i++ ) 
  {
    if(!strcmp(argv[i],"-h") || !strcmp(argv[i],"--help"))
    {
      Usage();
      exit(0);
    }
    else if(!strcmp(argv[i],"-v") || !strcmp(argv[i],"--version"))
    {
      puts("");
      exit(0);
    }
    else if(!strcmp(argv[i],"-s"))
    {
      if(++i<argc) 
      {
        strncpy(stage_io_directory, argv[i], sizeof(stage_io_directory));
        use_stage = true;
        printf("[Stage %s]", stage_io_directory );
      }
      else 
      {
        Usage();
        exit(-1);
      }
    }
    else if(!strcmp(argv[i], "-k"))
    {
      if(++i<argc) 
      { 
        strncpy(auth_key,argv[i],sizeof(auth_key));
        auth_key[sizeof(auth_key)-1] = '\0';
        printf("[Key %s]", auth_key);
      }
      else 
      {
        Usage();
        exit(-1);
      }
    }
    else if(!strcmp(argv[i], "-gerkey"))
    {
      printf("[gerkey]");
      player_gerkey = true;
    }
    else if(!strcmp(argv[i], "-d"))
    {
#if HAVE_LIB_DL
      if(++i<argc) 
      { 
        void* handle;
        fprintf(stderr,"Opening shared object %s...", argv[i]);
        if(!(handle = dlopen(argv[i], RTLD_NOW)))
        {
          fprintf(stderr,"\n  %s\n",dlerror());
          Interrupt(SIGINT);
        }
        else
          puts("Success!");
      }
      else 
      {
        Usage();
        exit(-1);
      }
#else
      PLAYER_ERROR("Sorry, no support for shared libraries");
      exit(-1);
#endif
    }
    else if(!strcmp(argv[i], "-p"))
    {
      if(++i<argc) 
      { 
        global_playerport = atoi(argv[i]);
	    
        printf("[Port %d]", global_playerport);
      }
      else 
      {
        Usage();
        exit(-1);
      }
    }
    else if(i == (argc-1))
    {
      // assume that this is a config file
      if(!parse_config_file(argv[i]))
        exit(-1);
    }
    else
    {
      Usage();
      exit(-1);
    }
  }

  puts( "" ); // newline, flush

  
  if( use_stage )
  {
    // create the shared memory connection to Stage
    // returns pointer to the timeval struct
    // and creates the ports array and array length with the port numbers
    // deduced from the stageIO filenames
    stage_clock_t * sclock = CreateStageDevices( stage_io_directory, 
						 &ports, &num_ufds );
    
    assert( sclock ); 
    //printf( "created %d ports (1: %d 2: %d...)\n",
    //    num_ufds, ports[0], ports[1] );

    // allocate storage for poll structures
    assert( ufds = new struct pollfd[num_ufds] );
    
#ifdef VERBOSE
    printf( "[Port" );
#endif

    // bind a socket on each port
    for(int i=0;i<num_ufds;i++)
    {
#ifdef VERBOSE
      printf( " %d", ports[i] ); fflush( stdout );
#endif      
      // setup the socket to listen on
      if((ufds[i].fd = create_and_bind_socket(&listener,1, ports[i], 
                                              SOCK_STREAM,200)) == -1)
      {
        fputs("create_and_bind_socket() failed; quitting", stderr);
        exit(-1);
      }
	
      ufds[i].events = POLLIN;
    }
    
#ifdef VERBOSE
    puts( "]" );
#endif

  }  

  // check for empty device table
  if(!(deviceTable->Size()))
  {
    if( use_stage )
      PLAYER_WARN("No devices instantiated; no valid Player devices in worldfile?");
    else
      PLAYER_WARN("No devices instantiated; perhaps you should supply " 
		  "a configuration file?");
  }

  if(!use_stage)
  {
    num_ufds = 1;
    if(!(ufds = new struct pollfd[num_ufds]) || !(ports = new int[num_ufds]))
    {
      perror("new failed for pollfds or ports");
      exit(-1);
    }

    // setup the socket to listen on
    if((ufds[0].fd = create_and_bind_socket(&listener,1,
                                            global_playerport,
                                            SOCK_STREAM,200)) == -1)
    {
      fputs("create_and_bind_socket() failed; quitting", stderr);
      exit(-1);
    }

    ufds[0].events = POLLIN;
    ports[0] = global_playerport;
  }

  // create the client manager object.
  clientmanager = new ClientManager(ufds,ports,num_ufds,auth_key);

  // main loop: sleep the shortest amount possible, periodically updating
  // the clientmanager.
  for(;;) 
  {
    //usleep(0);
    if(clientmanager->Update())
    {
      fputs("ClientManager::Update() errored; bailing.\n", stderr);
      exit(-1);
    }
  }

  /* don't get here */
  return(0);
}



