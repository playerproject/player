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

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if HAVE_LIBDL
  #include <dlfcn.h>
#endif

#include <sys/types.h>  /* for accept(2) */
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <string.h> 
#include <stdlib.h>  /* free(3),exit(3) */
#include <signal.h>
#include <unistd.h>  /* for close(2) */
#include <sys/socket.h>  /* for accept(2) */
#include <netdb.h> /* for gethostbyaddr(3) */
#include <netinet/in.h> /* for struct sockaddr_in, SOCK_STREAM */

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

//#include <fcntl.h>

#include "replace.h"

#if INCLUDE_STAGE
#include <sys/mman.h> // for mmap
#include <stagetime.h>
#include <stagedevice.h>
#endif

// Gazebo stuff
#if INCLUDE_GAZEBO
#include "gz_client.h"
#include "gz_time.h"
#endif

// Log file stuff
#if INCLUDE_LOGFILE
#include "readlog_manager.h"
#include "readlog_time.h"
#endif

//#define VERBOSE
//#define DEBUG

// true if the main loop should terminate
bool quit = false;

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
bool autoassign_ports = false;

int global_playerport = PLAYER_PORTNUM; // used to gen. useful output & debug

extern player_interface_t interfaces[];

/* Usage statement */
void
Usage()
{
  int maxlen=66;
  char** sortedlist;

  puts("");
  fprintf(stderr, "USAGE:  player [options] [<configfile>]\n\n");
  fprintf(stderr, "Where [options] can be:\n");
  fprintf(stderr, "  -h             : Print this message.\n");
  fprintf(stderr, "  -t {tcp | udp} : transport protocol to use.  Default: tcp\n");
  fprintf(stderr, "  -p <port>      : port where Player will listen. "
          "Default: %d\n", PLAYER_PORTNUM);
  fprintf(stderr, "  -s <path>      : use memory-mapped IO with Stage "
          "through the devices in\n                   this directory\n");
  fprintf(stderr, "  -g <path>      : connect to Gazebo instance at <path> \n");
  fprintf(stderr, "  -r <logfile>   : read data from <logfile> (readlog driver)\n");
  fprintf(stderr, "  -d <shlib>     : load the the indicated shared library\n");
  fprintf(stderr, "  -k <key>       : require client authentication with the "
          "given key\n");
  fprintf(stderr, "  <configfile>   : load the the indicated config file\n");
  fprintf(stderr, "\nThe following %d drivers were compiled into Player:\n\n    ",
          driverTable->Size());
  sortedlist = driverTable->SortDrivers();
  for(int i=0, len=0; i<driverTable->Size(); i++)
  {
    if((len += strlen(sortedlist[i])) >= maxlen)
    {
      fprintf(stderr,"\n    ");
      len=strlen(sortedlist[i]);
    }
    fprintf(stderr, "%s ", sortedlist[i]);
  }
  free(sortedlist);
  fprintf(stderr,"\n\nStage support was ");
#if ! INCLUDE_STAGE
  fprintf(stderr,"NOT ");
#endif
  fprintf(stderr,"included.\n");
  fprintf(stderr,"\nPart of the Player/Stage Project [http://playerstage.sourceforge.net].\n");
  fprintf(stderr, "Copyright (C) 2000 - 2003 Brian Gerkey, Richard Vaughan, Andrew Howard,\nand contributors.\n");
  fprintf(stderr,"\nReleased under the GNU General Public License.\n");
  fprintf(stderr,"\nPlayer comes with ABSOLUTELY NO WARRANTY.  This is free software, and you are\nwelcome to redistribute it under certain conditions; see COPYING for details.\n\n");
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
  if (dummy == SIGTERM)
  {
    printf("** Player [port %d] quitting **\n", global_playerport );
    exit(0);
   }
  
  // Tell the main loop to quit
  quit = 1;
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


#if INCLUDE_STAGE
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
void
CreateStageDevices(char *directory, int **ports, struct pollfd **ufds, 
                   int *num_ufds, int protocol)
{
#ifdef VERBOSE
  printf( "Searching for Stage devices\n" );
#endif

  struct dirent **namelist;
  int n = -1;
  int m;
  
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
    exit(-1);
  } 
 
  // open all the files in the IO directory
  n = scandir( directory, &namelist, MatchDeviceName, alphasort);
  if (n < 0)
    perror("scandir");
  else if(!n)
    PLAYER_WARN1("found no stage device files in directory \"%s\"", directory);
  else
  {
    // for every file in the directory, create a new device
    m=0;
    while(m<n)
    {
      // don't try to load the clock here - we'll do it below
      if( strcmp( STAGE_CLOCK_NAME, namelist[m]->d_name ) == 0 )
      {
        free(namelist[m]);
        m++;
        continue;
      }
      
      // don't try to open the lock here - we already did it above
      if( strcmp( STAGE_LOCK_NAME, namelist[m]->d_name ) == 0 )
      {
        free(namelist[m]);
        m++;
        continue;
      }

#ifdef DEBUG      
      printf("Opening %s ", namelist[m]->d_name);
      fflush( stdout );
#endif
      
      sprintf( devicefile, "%s/%s", directory, namelist[m]->d_name ); 
      
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
      
      // declare outside switch statement
      StageDevice *dev = 0; 
      //PSDevice* devicep = 0;

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
        case PLAYER_POWER_CODE:
        case PLAYER_BUMPER_CODE:
        case PLAYER_IR_CODE:
        {
          // Create a StageDevice with this IO base address and filedes
          dev = new StageDevice( deviceIO, lockfd, deviceIO->lockbyte );
	  
          deviceTable->AddDevice(deviceIO->player_id, 
                                 (char*)(deviceIO->drivername),
                                 (char*)(deviceIO->robotname),
                                 PLAYER_ALL_MODE, dev);
	  
          // add this port to our listening list
          StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
        }
        break;

        case PLAYER_LOCALIZE_CODE:
        {
          PLAYER_WARN("Localization drivers cannot be loaded from a .world "
                      "file.\nYou must run a separate instance of Player "
                      "and use the passthrough driver.");
        }
        break;

        case PLAYER_MCOM_CODE:
          // Create mcom device as per normal
          //
          // FIXME: apparently deviceIO->local is *not* currently being set
          // by Stage?...
          if(deviceIO->local || !deviceIO->local)
          {
            // no options in world file.
              
            // find the broadcast device in the available device table
            DriverEntry* entry;
            if(!(entry = driverTable->GetDriverEntry("lifomcom")))
            {
              puts("WARNING: Player support for mcom device unavailable.");
            }
            else
            {
              int section = configFile.AddEntity(globalparent, "mcom");
              // add it to the instantiated device table
              deviceTable->AddDevice(deviceIO->player_id, "mcom",
                                     (char*)(deviceIO->robotname),
                                     PLAYER_ALL_MODE, 
                                     (*(entry->initfunc))(PLAYER_MCOM_STRING,
                                                          &configFile, section));
 
              // add this port to our listening list
              StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
            }
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
            int section = configFile.AddEntity(globalparent,
                                               PLAYER_COMMS_STRING);
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
              deviceTable->AddDevice(deviceIO->player_id, "udpbroadcast",
                                     (char*)(deviceIO->robotname),
                                     PLAYER_ALL_MODE, 
                                     (*(entry->initfunc))(PLAYER_COMMS_STRING,
                                                          &configFile, section));
 
              // add this port to our listening list
              StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
            }
          }
          break;

        // devices not implemented
        case PLAYER_AUDIO_CODE:   
        case PLAYER_AUDIODSP_CODE:
        case PLAYER_AUDIOMIXER_CODE:
        case PLAYER_AIO_CODE:
        case PLAYER_DIO_CODE:
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
      free(namelist[m]);
      m++;
    }
    free(namelist);
  }
  

  // set the number of ports detected
  *num_ufds = portcount;

  // we've discovered all thew ports now, so allocate memory for the
  // required port numbers at the return pointer
  assert( *ports = new int[ *num_ufds ] );
  
  // copy the port numbers in
  memcpy( *ports, portstmp, *num_ufds * sizeof(int) );

  // clean up
  delete[] portstmp;

  struct sockaddr_in listener;
  
  //printf( "created %d ports (1: %d 2: %d...)\n",
  //    num_ufds, ports[0], ports[1] );
    
  // allocate storage for poll structures
  assert( *ufds = new struct pollfd[*num_ufds] );

#ifdef VERBOSE
  printf( "[Port ");
#endif

  if(!autoassign_ports)
  {
    // bind a socket on each port
    for(int i=0;i<*num_ufds;i++)
    {
#ifdef VERBOSE
      printf( " %d", (*ports)[i] ); fflush( stdout );
#endif      
      // setup the socket to listen on
      if(((*ufds)[i].fd = create_and_bind_socket(&listener,1, (*ports)[i], 
                                               protocol,200)) == -1)
      {
        fputs("create_and_bind_socket() failed; quitting", stderr);
        exit(-1);
      }

      (*ufds)[i].events = POLLIN;
    }
  }
  else
  {
    // we're supposed to auto-assign ports.  first, we must get the
    // baseport.  then we'll get whichever other ports we can.
    int curr_ufd = 0;
    int curr_port = global_playerport;
    CDeviceEntry* entry;

    puts("  Auto-assigning ports; Player is listening on the following ports:");
    printf("    [ ");
    fflush(stdout);
    while((curr_port < 65536) && (curr_ufd < *num_ufds))
    {
      if(((*ufds)[curr_ufd].fd = 
          create_and_bind_socket(&listener,1,curr_port,
                                 protocol,200)) == -1)
      {
        if(!curr_ufd)
        {
          PLAYER_ERROR1("couldn't get base port %d", global_playerport);
          exit(-1);
        }
      }
      else
      {
        printf("%d ", curr_port);
        fflush(stdout);
        (*ufds)[curr_ufd].events = POLLIN;
        // now, go through the device table and change all references to
        // this port.
        for(entry = deviceTable->GetFirstEntry();
            entry;
            entry = deviceTable->GetNextEntry(entry))
        {
          if(entry->id.port == (*ports)[curr_ufd])
            entry->id.port = curr_port;
        }
        // also have to fix the port list
        (*ports)[curr_ufd] = curr_port;
        curr_ufd++;
      }
      curr_port++;
    }

    if(curr_port == 65536)
    {
      PLAYER_ERROR("couldn't find enough free ports");
      exit(-1);
    }

    puts("]");
  }
    
#ifdef VERBOSE
  puts( "]" );
#endif

  return;
}

#endif

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
  printf("\nParsing configuration file \"%s\"...\n", fname);

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
      deviceTable->AddDevice(id, driver, NULL, entry->access, tmpdevice);

      // should this device be "always on"?
      if(configFile.ReadInt(i, "alwayson", 0))
      {
        // In order to allow safe shutdown, we need to create a dummy
        // clientdata object and add it to the clientmanager.  It will then
        // form a root for this subscription tree and allow it to be torn
        // down.
        ClientData* clientdata;
        player_device_req_t req;

        assert((clientdata = (ClientData*)new ClientDataTCP("",0)));
        
        // to indicate that this one is a dummy
        clientdata->socket = 0;

        clientmanager->AddClient(clientdata);
        req.code = code;
        req.index = index;
        req.access = PLAYER_READ_MODE;
        if(clientdata->UpdateRequested(req) != PLAYER_READ_MODE)
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
  char auth_key[PLAYER_KEYLEN] = "";
  char *configfile = NULL;
  char *gz_serverid = NULL;
  char *readlog_filename = NULL;
  double readlog_speed = 1.0;
  
  int *ports = NULL;
  struct pollfd *ufds = NULL;
  int num_ufds = 0;
  int protocol = PLAYER_TRANSPORT_TCP;
  char stage_io_directory[MAX_FILENAME_SIZE]; // filename for mapped memory

  // Register the available drivers in the driverTable.  
  //
  // Although most of these devices are only used with physical devices, 
  // some (e.g., broadcast, bps) are used with Stage, and so we always call 
  // this function.
  register_devices();
  
  // Trap ^C
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
#if INCLUDE_STAGE
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
#else
      PLAYER_ERROR("Sorry, support for Stage not included at compile-time.");
      exit(-1);
#endif
    }

    // Gazebo support
    else if(!strcmp(argv[i], "-g"))
    {
      if(++i<argc)
      {
        gz_serverid = argv[i];
      }
      else
      {
        Usage();
        exit(-1);
      }
    }

    // ReadLog support
    else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--readlog"))
    {
      if(++i<argc)
      {
        readlog_filename = argv[i];
      }
      else
      {
        Usage();
        exit(-1);
      }
    }

    // ReadLog support
    else if(!strcmp(argv[i], "-f") || !strcmp(argv[i], "--readlogspeed"))
    {
      if(++i<argc)
      {
        readlog_speed = atof(argv[i]);
      }
      else
      {
        Usage();
        exit(-1);
      }
    }

    // Authorization key
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
    else if(!strcmp(argv[i], "-t"))
    {
      if(++i<argc) 
      { 
        if(!strcmp(argv[i], "tcp"))
          protocol = PLAYER_TRANSPORT_TCP;
        else if(!strcmp(argv[i], "udp"))
          protocol = PLAYER_TRANSPORT_UDP;
        else
        {
          printf("\nUnknown transport protocol \"%s\"\n", argv[i]);
          Usage();
          exit(-1);
        }
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
#if HAVE_LIBDL
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
    else if(!strcmp(argv[i], "-a"))
    {
      autoassign_ports = true;
    }
    else if(i == (argc-1))
    {
      // assume that this is a config file
      configfile = argv[i];
    }
    else
    {
      Usage();
      exit(-1);
    }
  }

  printf(" [%s]", (protocol == PLAYER_TRANSPORT_TCP) ? "TCP" : "UDP");

  puts( "" ); // newline, flush


  if (use_stage)
  {
#if INCLUDE_STAGE
    // Use the clock from State
    GlobalTime = new StageTime(stage_io_directory);
    assert(GlobalTime);
#else
    PLAYER_ERROR("Sorry, support for Stage not included at compile-time.");
    exit(-1);
#endif
  }
  else if (gz_serverid != NULL)
  {
#ifdef INCLUDE_GAZEBO
    // Initialize gazebo client
    if (GzClient::Init(gz_serverid) != 0)
      exit(-1);

    // Use the clock from Gazebo
    GlobalTime = new GzTime();
    assert(GlobalTime);
#else
    
    PLAYER_ERROR("Sorry, support for Gazebo not included at compile-time.");
    exit(-1);
#endif
  }
  else if (readlog_filename != NULL)
  {
#ifdef INCLUDE_LOGFILE
    // Initialize the readlog reader
    if (ReadLogManager_Init(readlog_filename, readlog_speed) != 0)
      exit(-1);

    // Use the clock from the log file
    GlobalTime = new ReadLogTime();
    assert(GlobalTime);
#else
    PLAYER_ERROR("Sorry, support for log files not included at compile-time.");
    exit(-1);
#endif
  }
  else
  {
    // Use the system clock
    GlobalTime = new WallclockTime();
    assert(GlobalTime);
  }

  // Instantiate devices
  if (use_stage)
  {
#if INCLUDE_STAGE
    // create the shared memory connection to Stage
    // returns pointer to the timeval struct
    // and creates the ports array and array length with the port numbers
    // deduced from the stageIO filenames
    CreateStageDevices( stage_io_directory, &ports, &ufds, &num_ufds, protocol);
#endif
  }
  else
  {
    num_ufds = 1;
    assert((ufds = new struct pollfd[num_ufds]) && 
           (ports = new int[num_ufds]));

    struct sockaddr_in listener;

    // setup the socket to listen on
    if((ufds[0].fd = create_and_bind_socket(&listener,1,global_playerport,
                                            protocol,200)) == -1)
    {
      fputs("create_and_bind_socket() failed; quitting", stderr);
      exit(-1);
    }

    ufds[0].events = POLLIN;
    ports[0] = global_playerport;
  }
  
  // create the client manager object.
  if(protocol == PLAYER_TRANSPORT_TCP)
    clientmanager = (ClientManager*)(new ClientManagerTCP(ufds, ports, 
                                                          num_ufds, auth_key));
  else if(protocol == PLAYER_TRANSPORT_UDP)
    clientmanager = (ClientManager*)(new ClientManagerUDP(ufds, ports, 
                                                          num_ufds, auth_key));
  else
  {
    PLAYER_ERROR("Unknown transport protocol");
    exit(-1);
  }


  if(configfile != NULL)
  {
    // Parse the config file and instantiate drivers
    if (!parse_config_file(configfile))
      exit(-1);
  }

  // check for empty device table
  if(!(deviceTable->Size()))
  {
    if( use_stage )
      PLAYER_ERROR("No devices instantiated; no valid Player devices in worldfile?");
    else
      PLAYER_ERROR("No devices instantiated; perhaps you should supply " 
                  "a configuration file?");
    exit(-1);
  }

  // main loop: sleep the shortest amount possible, periodically updating
  // the clientmanager.
  while (!quit)
  {
    usleep(0);
    if(clientmanager->Update())
    {
      fputs("ClientManager::Update() errored; bailing.\n", stderr);
      exit(-1);
    }
  }

  if(use_stage)
    puts("** Player quitting **" );
  else
    printf("** Player [port %d] quitting **\n", global_playerport );

#if INCLUDE_LOGFILE
  // Finalize ReadLog manager
  if (readlog_filename != NULL)
    ReadLogManager_Fini();
#endif

#if INCLUDE_GAZEBO
  // Finalize gazebo client
  if (gz_serverid != NULL)
    GzClient::Fini();
#endif
  
  // tear down the client table, which shuts down all open devices
  delete clientmanager;
  // tear down the device table, for completeness
  delete deviceTable;
  // tear down the driver table, for completeness
  delete driverTable;
  
  return(0);
}



