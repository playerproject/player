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


// I can't find a way to do @prefix@ substitution in server/prefix.h that
// works in more than one version of autotools.  Fix this later.
//#include <prefix.h>

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

#if INCLUDE_STAGE1P4
#include "stg_time.h"
#endif

//#define VERBOSE
//#define DEBUG

// true if the main loop should terminate
bool quit = false;

// true if sigint should be ignored
bool mask_sigint = false;

// true if we're connecting to Stage instead of a real robot
bool use_stage = false;

// enable "special" extensions
bool player_gerkey = false;

size_t ioSize = 0; // size of the IO buffer

// pid of the main server thread.  used in CDevice::DataAvailable() to
// send a SIGUSR1 to this thread when new data is available
//pid_t g_server_pid;

// this table holds all the currently *available* drivers
DriverTable* driverTable = new DriverTable();

// this table holds all the currently *instantiated* devices
DeviceTable* deviceTable = new DeviceTable();

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

// Try to load a given plugin, using a particular search algorithm.
// Returns true on success and false on failure.
bool LoadPlugin(const char* pluginname, const char* cfgfile);

/** @page commandline Command line options

The Player server is run as follows:

@verbatim
$ player [options] <configfile>
@endverbatim

where [options] is one or more of the following:

- -h             : Print usage message.
- -t {tcp | udp} : transport protocol to use.  Default: tcp.
- -p &lt;port&gt;      : port where Player will listen. Default: 6665.
- -s &lt;path&gt;      : use memory-mapped IO with Stage through the devices in this directory.
- -g &lt;id&gt;        : connect to Gazebo server with id &lt;id&gt;.
- -r &lt;logfile&gt;   : read data from &lt;logfile&gt; (readlog driver).
- -k &lt;key&gt;       : require client authentication with the given key.

Note that only one of -s, -g and -r can be specified at any given time.

*/
void Usage()
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
  if (mask_sigint == 0)
    quit = 1;
}

void
DoNothing(int sig)
{
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

  if(signal(SIGUSR1, DoNothing) == SIG_ERR)
  {
    perror("signal(2) failed while setting up for SIGUSR1");
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
      //int globalparent = configFile.AddEntity(-1,"");
      

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
	  
          /*
          if(deviceTable->AddDevice(deviceIO->player_id, 
                                    (char*)(deviceIO->drivername),
                                    (char*)(deviceIO->robotname),
                                    PLAYER_ALL_MODE, dev) < 0)
                                    */
          
          // add this port to our listening list
          StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
        }
        break;

#if 0
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
              if(deviceTable->AddDevice(deviceIO->player_id, "mcom",
                                        (char*)(deviceIO->robotname),
                                        PLAYER_ALL_MODE, 
                                        (*(entry->initfunc))(PLAYER_MCOM_STRING,
                                                             &configFile, 
                                                             section)) < 0)
                exit(-1);
 
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
              if(deviceTable->AddDevice(deviceIO->player_id, "udpbroadcast",
                                        (char*)(deviceIO->robotname),
                                        PLAYER_ALL_MODE, 
                                        (*(entry->initfunc))(PLAYER_COMMS_STRING,
                                                             &configFile, 
                                                             section)) < 0)
                exit(-1);
 
              // add this port to our listening list
              StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
            }
          }
          break;

        case PLAYER_SERVICE_ADV_CODE:
        {
            DriverEntry* entry = driverTable->GetDriverEntry("service_adv_mdns");
            if(!entry) {
                puts("WARNING: Player support for service_adv_mdns is unavailable!");
                break;
            }
            int section = configFile.AddEntity(globalparent, PLAYER_SERVICE_ADV_STRING);
            // TODO: maybe get some stuff out of the world file and add it here?

            if(deviceTable->AddDevice(deviceIO->player_id, 
                                      PLAYER_SERVICE_ADV_STRING,
                                      (char*) deviceIO->robotname, PLAYER_ALL_MODE,
                                      (* (entry->initfunc) ) (PLAYER_SERVICE_ADV_STRING, 
                                                              &configFile, 
                                                              section)) > 0)
              exit(-1);

            StageAddPort(portstmp, &portcount, deviceIO->player_id.port);
            break;
        }
#endif

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
    puts("Sorry, auto-assiging port disabled for now.  Come again.");
    exit(-1);
#if 0
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
#endif
  }
    
#ifdef VERBOSE
  puts( "]" );
#endif

  return;
}

#endif

// Try to load a given plugin, using a particular search algorithm.
// Returns true on success and false on failure.
bool
LoadPlugin(const char* pluginname, const char* cfgfile)
{
#if HAVE_LIBDL
  void* handle=NULL;
  PluginInitFn initfunc;
  char fullpath[PATH_MAX];
  char* playerpath;
  char* tmp;
  char* cfgdir;
  char* error;
  unsigned int i,j;

  // see if we got an absolute path
  if(pluginname[0] == '/' || pluginname[0] == '~')
  {
    strcpy(fullpath,pluginname);
    printf("trying to load %s...", fullpath);
    fflush(stdout);
    if((handle = dlopen(fullpath, RTLD_NOW)))
      puts("success");
    else
    {
      puts("failed");
      return(false);
    }
  }

  // we got a relative path, so search for the module

  // did the user set PLAYERPATH?
  if(!handle && (playerpath = getenv("PLAYERPATH")))
  {
    printf("PLAYERPATH: %s\n", playerpath);
    // yep, now parse it, as a colon-separated list of directories
    i=0;
    while(i<strlen(playerpath))
    {
      j=i;
      while(j<strlen(playerpath))
      {
        if(playerpath[j] == ':')
        {
          break;
        }
        j++;
      }
      memset(fullpath,0,PATH_MAX);
      strncpy(fullpath,playerpath+i,j-i);
      strcat(fullpath,"/");
      strcat(fullpath,pluginname);
      printf("trying to load %s...", fullpath);
      fflush(stdout);
      if((handle = dlopen(fullpath, RTLD_NOW)))
      {
        puts("success");
        break;
      }
      else
        puts("failed");

      i=j+1;
    }
  }
  
  // try to load it from the directory where the config file is
  if(!handle && cfgfile)
  {
    // Note that dirname() modifies the contents, so
    // we need to make a copy of the filename.
    tmp = strdup(cfgfile);
    memset(fullpath,0,PATH_MAX);
    cfgdir = dirname(tmp);
    if(cfgdir[0] != '/' && cfgdir[0] != '~')
    {
      getcwd(fullpath, PATH_MAX);
      strcat(fullpath,"/");
    }
    strcat(fullpath,cfgdir);
    strcat(fullpath,"/");
    strcat(fullpath,pluginname);
    free(tmp);
    printf("trying to load %s...", fullpath);
    fflush(stdout);
    if((handle = dlopen(fullpath, RTLD_NOW)))
      puts("success");
    else
      puts("failed");
  }

// I can't find a way to do @prefix@ substitution in server/prefix.h that
// works in more than one version of autotools.  Fix this later.
#if 0
  // try to load it from prefix/lib/player/plugins
  if(!handle)
  {
    memset(fullpath,0,PATH_MAX);
    strcpy(fullpath,PLAYER_INSTALL_PREFIX);
    strcat(fullpath,"/lib/player/plugins/");
    strcat(fullpath,pluginname);
    printf("trying to load %s...", fullpath);
    fflush(stdout);
    if((handle = dlopen(fullpath, RTLD_NOW)))
      puts("success");
    else
      puts("failed");

  }
#endif

  if (handle == NULL)
  {
    PLAYER_ERROR1("error loading plugin: %s", dlerror());
    return false;
  }
  
  // Now invoke the initialization function
  if(handle)
  {
    printf("invoking player_driver_init()...");
    fflush(stdout);
    initfunc = (PluginInitFn)dlsym(handle,"player_driver_init");
    if((error = dlerror()) != NULL)
    {
      printf("failed: %s\n", error);
      return(false);
    }
    if((*initfunc)(driverTable) != 0)
    {
      puts("failed");
      PLAYER_ERROR1("error loading plugin: %s", dlerror());
      return(false);
    }
    puts("success");
    return(true);
  }
  else
    return(false);

#else
  PLAYER_ERROR("Sorry, no support for shared libraries, so can't load plugins");
  return false;
#endif
}


// Parse a new-style device from the config file
bool ParseDeviceEx(ConfigFile *cf, int section)
{
  const char *pluginname;  
  const char *drivername;
  DriverEntry *entry;
  Driver *driver;
  Device *device;

  entry = NULL;
  driver = NULL;
  
  // Load any required plugins
  pluginname = cf->ReadString(section, "plugin", NULL);
  if (pluginname != NULL)
  {
    if(!LoadPlugin(pluginname,cf->filename))
    {
      PLAYER_ERROR1("failed to load plugin: %s", pluginname);
      return (false);
    }
  }

  // Get the driver name
  drivername = cf->ReadString(section, "driver", NULL);
  if (drivername == NULL)
  {
    PLAYER_ERROR1("No driver name specified in section %d", section);
    return (false);
  }
  
  // Look for the driver
  entry = driverTable->GetDriverEntry(drivername);
  if (entry == NULL)
  {
    PLAYER_ERROR1("Couldn't find driver \"%s\"", drivername);
    return (false);
  }

  // Create a new-style driver
  if (entry->initfunc == NULL)
  {
    PLAYER_ERROR1("Driver has no initialization function \"%s\"", drivername);
    return (false);
  }

  // Create a driver; the driver will add entries into the device
  // table
  driver = (*(entry->initfunc)) (cf, section);
  if (driver == NULL || driver->error != 0)
  {
    PLAYER_ERROR1("Initialization failed for driver \"%s\"", drivername);
    return (false);
  }

  // Fill out the driver name in the device table
  for (device = deviceTable->GetFirstDevice(); device != NULL;
       device = deviceTable->GetNextDevice(device))
  {
    if (device->driver == driver)
      strncpy(device->drivername, drivername, sizeof(device->drivername));
  }
  
  // Should this device be "always on"?
  if (driver)
    driver->alwayson = cf->ReadInt(section, "alwayson", 0);

  return true;
}


// Display the driver/interface map
void PrintDeviceTable()
{
  Driver *last_driver;
  Device *device;
  player_interface_t interface;

  last_driver = NULL;
  printf("------------------------------------------------------------\n");
  
  // Step through the device table
  for (device = deviceTable->GetFirstDevice(); device != NULL;
       device = deviceTable->GetNextDevice(device))
  {
    assert(lookup_interface_code(device->id.code, &interface) == 0);
    
    if (device->driver != last_driver)
    {
      fprintf(stdout, "%d driver %s id %d:%s:%d\n",
              device->index, device->drivername,
              device->id.port, interface.name, device->id.index);
    }
    else
    {
      fprintf(stdout, "%d        %*s id %d:%s:%d\n",
              device->index, strlen(device->drivername), "",
              device->id.port, interface.name, device->id.index);
    }
    last_driver = device->driver;
  }

  printf("------------------------------------------------------------\n");
  return;
}



bool
ParseConfigFile(char* fname, int** ports, int* num_ports, 
                  ClientData*** alwayson_devices, int* num_alwayson_devices)
{
  //DriverEntry* entry;
  //Driver* tmpdriver;
  //char interface[PLAYER_MAX_DEVICE_STRING_LEN];
  //char* colon;
  //char* colon2;
  //int index;
  //char* driver;
  //const char *pluginname;  
  //void* handle;
  //int code = 0;
  int port = global_playerport;
  char robotname[PLAYER_MAX_DEVICE_STRING_LEN];
  //bool firstdevice=true;

  robotname[0]='\0';

  
  // parse the file
  printf("\nParsing configuration file \"%s\"\n", fname);

  if(!configFile.Load(fname))
    return(false);
  
  // a safe upper bound on the number of ports we'll need is the number of
  // entities in the config file (yes, i'm too lazy to dynamically
  // reallocate this buffer for a tighter bound).
  assert(*ports = new int[configFile.GetEntityCount()]);
  assert(*alwayson_devices = new ClientData*[configFile.GetEntityCount()]);
  // we'll increment this counter as we go
  *num_ports=0;

  // load each device specified in the file
  for(int i = 1; i < configFile.GetEntityCount(); i++)
  {
    if(configFile.entities[i].type < 0)
      continue;

    // Check for new-style device block
    if (strcmp(configFile.GetEntityType(i), "driver") == 0)
    {
      if (!ParseDeviceEx(&configFile, i))
        return false;
    }

    /* REMOVE/FIX
    // Load old-style device blocks
    // TODO: remove the port addition stuff; the port list
    // is now extracted directly from the devicetable (see below)
    else
    {
      // get the interface name
      strncpy(interface, (const char*)(configFile.entities[i].type), 
              sizeof(interface));
      interface[sizeof(interface)-1] = '\0';

      // look for a colon and trailing index
      if((colon = strchr(interface,':')))
      {
        // strip off the end
        //interface[colon-interface] = '\0';
        *colon = '\0';
      }
      // look for the new robot:<port>:<name> syntax
      if(!strcmp(interface,"robot"))
      {
        port = -1;
        robotname[0]='\0';
        if(colon && strlen(colon+1))
        {
          if((colon2 = strchr(colon+1,':')))
          {
            // strip off the end
            *colon2 = '\0';
            if(strlen(colon2+1))
            {
              strncpy(robotname,colon2+1,sizeof(robotname));
              robotname[sizeof(robotname)-1]='\0';
            }
          }
          // was a port number actually supplied?
          if(strlen(colon+1))
          {
            port = atoi(colon+1);
            // add this port to our watchlist.  could check for duplicates
            // here, but we'll just let bind() do that checking for us later
            (*ports)[(*num_ports)++] = port;
          }
        }
        continue;
      }
      else
      {
        // get the index out
        if(colon && strlen(colon+1))
          index = atoi(colon+1);
        else
          index = 0;

        // if we're using the old single robot syntax (i.e., no robot blocks)
        // and this is the first device, then we have to add the single port
        // to our watchlist
        if(firstdevice && (*num_ports == 0))
          (*ports)[(*num_ports)++] = port;
      }

      // TODO: implement auto-assignment of ports
      if(port < 0)
      {
        PLAYER_ERROR("Sorry, auto-assigning ports not yet supported");
        return(false);
      }

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
        return(false);
      }

      // did the user specify a different driver?
      driver = (char*)configFile.ReadString(i, "driver", driver);

      // Load any required plugins
      pluginname = configFile.ReadString(i, "plugin", NULL);
      if (pluginname != NULL)
      {
        if(!LoadPlugin(pluginname,configFile.filename))
        {
          PLAYER_ERROR1("failed to load plugin: %s", pluginname);
          return (false);
        }
      }

      printf("  loading driver \"%s\" as device \"%d:%s:%d\"\n", 
             driver, port, interface, index);
      // look for the indicated driver in the available device table
      if(!(entry = driverTable->GetDriverEntry(driver)))
      {
        PLAYER_ERROR1("Couldn't find driver \"%s\"", driver);
        return(false);
      }
      else
      {
        player_device_id_t id;
        id.code = code;
        id.port = port;
        id.index = index;

        // Create the driver
        if(!(tmpdriver = (*(entry->initfunc))(interface,&configFile,i)))
        {
          PLAYER_ERROR2("Initialization failed for driver \"%s\" as interface \"%s\"\n",
                        driver, interface);
          return(false);
        }

        // Add driver to the device table
        if (deviceTable->AddDevice(id, driver, robotname, entry->access, tmpdriver) != 0)
          return(false);

        firstdevice=false;

        // should this device be "always on"?
        if(configFile.ReadInt(i, "alwayson", 0))
          tmpdriver->alwayson = true;
      }
    }
    */      

  }

  // Warn of any unused variables
  configFile.WarnUnused();

  // Print the device table
  puts("Using device table:");
  PrintDeviceTable();

  int i;
  Device *device;

  // Poll the device table for ports to monitor
  for (device = deviceTable->GetFirstDevice(); device != NULL;
       device = deviceTable->GetNextDevice(device))
  {
    // See if the port is already in the table
    for (i = 0; i < *num_ports; i++)
    {
      if ((*ports)[i] == device->id.port)
        break;
    }

    // If its not in the table, add it
    if (i >= *num_ports)
      (*ports)[(*num_ports)++] = device->id.port;
  }

  // Poll the device table for alwayson devices
  for (device = deviceTable->GetFirstDevice(); device != NULL;
       device = deviceTable->GetNextDevice(device))
  {
    if (!device->driver->alwayson)
      continue;
    
    // In order to allow safe shutdown, we need to create a dummy
    // clientdata object and add it to the clientmanager.  It will then
    // form a root for this subscription tree and allow it to be torn
    // down.
    ClientData* clientdata;
    player_device_req_t req;

    assert((clientdata = (ClientData*)new ClientDataTCP("",port)));
        
    // to indicate that this one is a dummy
    clientdata->socket = 0;

    // don't add it to the client manager here, because it hasn't been
    // created yet.  instead, queue them up for later addition.
    //clientmanager->AddClient(clientdata);
    (*alwayson_devices)[(*num_alwayson_devices)++] = clientdata;
        
    req.code = device->id.code;
    req.index = device->id.index;
    req.access = PLAYER_READ_MODE;
    if(clientdata->UpdateRequested(req) != PLAYER_READ_MODE)
    {
      PLAYER_ERROR2("Initial subscription failed to driver \"%s\" as interface \"%s\"\n",
                    device->drivername, lookup_interface_name(0, device->id.code));
      return(false);
    }
  }

  return(true);
}

// some drivers use libraries that need the arguments for initialization
int global_argc;
char** global_argv;

int main( int argc, char *argv[] )
{
  char auth_key[PLAYER_KEYLEN] = "";
  char *configfile = NULL;
  char *gz_serverid = NULL;
  char *gz_prefixid = NULL;
  char *readlog_filename = NULL;
  double readlog_speed = 1.0;
  
  int *ports = NULL;
  ClientData** alwayson_devices = NULL;
  int num_alwayson_devices = 0;
  struct pollfd *ufds = NULL;
  int num_ufds = 0;
  int protocol = PLAYER_TRANSPORT_TCP;
#if INCLUDE_STAGE
  char stage_io_directory[MAX_FILENAME_SIZE]; // filename for mapped memory
#endif

  global_argc = argc;
  global_argv = argv;

  // Register the available drivers in the driverTable.  
  //
  // Although most of these devices are only used with physical devices, 
  // some (e.g., broadcast, bps) are used with Stage, and so we always call 
  // this function.
  register_devices();
  
  // Trap ^C
  SetupSignalHandlers();

  //g_server_pid = getpid();

  printf("** Player v%s (with multiple interface support)** ", playerversion);
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
    else if(!strcmp(argv[i], "--gazebo-prefix"))
    {
      if(++i<argc)
      {
        gz_prefixid = argv[i];
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

    // Mask (ignore) SIGINT
    else if(!strcmp(argv[i], "--nosigint"))
    {
      printf("[nosigint]");
      mask_sigint = true;
    }
    else if(!strcmp(argv[i], "-gerkey"))
    {
      printf("[gerkey]");
      player_gerkey = true;
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
    if (GzClient::Init(gz_serverid, gz_prefixid) != 0)
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

  if(configfile != NULL)
  {
    // Parse the config file and instantiate drivers.  It builds up a list
    // of ports that we need to listen on.
    if (!ParseConfigFile(configfile, &ports, &num_ufds,
                         &alwayson_devices, &num_alwayson_devices))
      exit(-1);
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
    assert(ufds = new struct pollfd[num_ufds]);

    struct sockaddr_in listener;

    for(int i=0;i<num_ufds;i++)
    {
      // setup the socket to listen on
      if((ufds[i].fd = create_and_bind_socket(&listener,1,ports[i],
                                              protocol,200)) == -1)
      {
        PLAYER_ERROR("create_and_bind_socket() failed; quitting");
        exit(-1);
      }
      ufds[i].events = POLLIN;
      printf("listening on port %d\n", ports[i]);
    }
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

  // now that we've create the clientmanager, add any "alwayson" devices
  for(int i=0;i<num_alwayson_devices;i++)
    clientmanager->AddClient(alwayson_devices[i]);

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

  // give the devices one last chance to get ready, then it's damn the
  // torpedoes, etc.
  // WARNING: this feature is experimental and may be removed in the future
  for(Device* dev = deviceTable->GetFirstDevice(); 
          dev != 0; 
          dev = deviceTable->GetNextDevice(dev))
  {
      dev->driver->Prepare();
  }

  // main loop: sleep the shortest amount possible, periodically updating
  // the clientmanager.
  while (!quit)
  {
    //usleep(0);
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
  delete GlobalTime;
  
  return(0);
}



