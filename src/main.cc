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

#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <errno.h>
#include <string.h> // for bzero()
#include <stdlib.h>  /* atoi(3) */
#include <signal.h>
#include <pthread.h>  /* for pthread stuff */
#include <netinet/in.h> /* for struct sockaddr_in, SOCK_STREAM */
#include <unistd.h>  /* for close(2) */
#include <sys/types.h>  /* for accept(2) */
#include <sys/socket.h>  /* for accept(2) */
#include <sys/poll.h>  /* for poll(2) */
#include <netdb.h> /* for gethostbyaddr(3) */
#include <pubsub_util.h> /* for create_and_bind_socket() */

#include <defaults.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

#ifdef INCLUDE_LASER
#include <laserdevice.h>
#endif
#ifdef INCLUDE_SONAR
#include <sonardevice.h>
#endif
#ifdef INCLUDE_VISION
#include <visiondevice.h>
#endif
#ifdef INCLUDE_POSITION
#include <positiondevice.h>
#endif
#ifdef INCLUDE_GRIPPER
#include <gripperdevice.h>
#endif
#ifdef INCLUDE_MISC
#include <miscdevice.h>
#endif
#ifdef INCLUDE_PTZ
#include <ptzdevice.h>
#endif
#ifdef INCLUDE_AUDIO
#include <audiodevice.h>
#endif
#ifdef INCLUDE_LASERBEACON
#include <laserbeacondevice.hh>
#endif
#ifdef INCLUDE_BROADCAST
#include <broadcastdevice.hh>
#endif
#ifdef INCLUDE_SPEECH
#include <speechdevice.h>
#endif
#ifdef INCLUDE_BPS
#include <bpsdevice.h>
#endif

#include <clientdata.h>
#include <clientmanager.h>
#include <devicetable.h>

// the base class and two derived classes for timefeeds:
#include <playertime.h>
#include <wallclocktime.h>
#include <stagetime.h>

#include <sys/mman.h> // for mmap
#include <fcntl.h>

#ifdef INCLUDE_STAGE
#include <stagedevice.h>
player_stage_info_t *arenaIO; //address for memory mapped IO to Stage
char stage_io_directory[MAX_FILENAME_SIZE]; // filename for mapped memory
#endif

// true if we're connecting to Stage instead of a real robot
bool use_stage = false;

// enable "special" extensions
bool player_gerkey = false;

size_t ioSize = 0; // size of the IO buffer

CDeviceTable* deviceTable = new CDeviceTable();

// the global PlayerTime object has a method 
//   int GetTime(struct timeval*)
// which everyone must use to get the current time
PlayerTime* GlobalTime;

// keep track of the pointers to our various clients.
// that way we can cancel them at Shutdown
ClientManager* clientmanager;

// for use in other places (cliendata.cc, for example)
char playerversion[] = PLAYER_VERSION;

bool experimental = false;
bool debug = false;

int global_playerport = PLAYER_PORTNUM; // used to gen. useful output & debug

char* sane_spec[] = { "-misc:0",
                      "-gripper:0",
                      "-position:0",
                      "-sonar:0",
                      "-laser:0",
                      "-vision:0",
                      "-ptz:0",
                      "-laserbeacon:0",
                      "-broadcast:0",
                      "-speech:0",
                      "-bps:0"};

/* Usage statement */
void
Usage()
{
  puts("");
  fprintf(stderr, "USAGE: player [-port <port>] [-stage <path>] [-sane] "
          "[DEVICE]...\n");
  fprintf(stderr, "  -port <port>  : TCP port where Player will listen. "
          "Default: %d\n", PLAYER_PORTNUM);
  fprintf(stderr, "  -stage <path> : use memory-mapped IO with Stage "
          "through the devices in this directory\n");
  fprintf(stderr, "  -sane         : use the compiled-in device defaults:\n");
  for(int i=0;i<ARRAYSIZE(sane_spec);i++)
    fprintf(stderr, "                      %s\n",sane_spec[i]);
  fprintf(stderr, "\n  Each [DEVICE] should be of the form:\n");
  fprintf(stderr, "    -<name>:<index> [options]\n");
  fprintf(stderr, "  If omitted, <index> is assumed to be 0.\n");
  fprintf(stderr, "  [options] is a string of device-specific options.\n");
  fprintf(stderr, "  For example:\n");
  fprintf(stderr, "    -laser:0 \"port /dev/ttyS0\"\n");
}

/* sighandler to shut everything down properly */
void Interrupt( int dummy ) 
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

/* used to name incoming client connections */
int
make_dotted_ip_address(char* dest, int len, uint32_t addr)
{
  char tmp[512];
  int mask = 0xff;

  sprintf(tmp, "%u.%u.%u.%u",
                  addr>>0 & mask,
                  addr>>8 & mask,
                  addr>>16 & mask,
                  addr>>24 & mask);

  if((strlen(tmp) + 1) > (unsigned int)len)
  {
    return(-1);
  }
  else
  {
    strncpy(dest, tmp, len);
    return(0);
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

// a matching function to indentify valid device names
// used by scandir to fetch device filenames
int MatchDeviceName( const struct dirent* ent )
{
  // device names are > 2 chars long,; . and .. are not
  return( strlen( ent->d_name ) > 2 );
}

#ifdef INCLUDE_STAGE
// looks in the directory for device entries, creates the devices
// and fills an array with unique port numbers
stage_clock_t* CreateStageDevices( char* directory, int** ports, 
                                   int* num_ports )
{
#ifdef VERBOSE
  printf( "Searching for Stage devices\n" );
#endif

  struct dirent **namelist;
  int n;
  
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
  else
  {
    // for every file in the directory, create a new device
    while(n--)
    {

      // don't try to load the clock here - we'll do it below
      if( strcmp( STAGE_CLOCK_NAME, namelist[n]->d_name ) == 0 )
        break;

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
      
      CStageDevice *dev = 0; // declare outside switch statement

      // get the player type and index from the header
      // NOT from the filename
      switch( deviceIO->player_id.type )
      {
        //create a generic stage IO device for these types:
        case PLAYER_PLAYER_CODE: 
        case PLAYER_MISC_CODE:
        case PLAYER_POSITION_CODE:
        case PLAYER_SONAR_CODE:
        case PLAYER_LASER_CODE:
        case PLAYER_VISION_CODE:  
        case PLAYER_PTZ_CODE:     
        case PLAYER_LASERBEACON_CODE: 
        case PLAYER_TRUTH_CODE:
        case PLAYER_OCCUPANCY_CODE:
        case PLAYER_GPS_CODE:
        case PLAYER_GRIPPER_CODE:
        case PLAYER_IDAR_CODE:
        case PLAYER_DESCARTES_CODE:
        case PLAYER_MOTE_CODE:
        {
          // Create a StageDevice with this IO base address and filedes
          dev = new CStageDevice( deviceIO, lockfd, deviceIO->lockbyte );
	    
          /*
          printf("Stage adding device %d:%d:%d\n",
                 deviceIO->player_id.port,
                 deviceIO->player_id.type, 
                 deviceIO->player_id.index);
           */

          deviceTable->AddDevice( deviceIO->player_id.port,
                                  deviceIO->player_id.type, 
                                  deviceIO->player_id.index, 
                                  PLAYER_ALL_MODE, dev );
	    
          // have we registered this port already?
          int j = 0;
          while( j<portcount )
          {
            if( portstmp[j] == deviceIO->player_id.port )
              break;
            j++;
          }
          if( j == portcount ) // we have not! 
            // we add this port to the array of ports we must listen on
            portstmp[portcount++] = deviceIO->player_id.port;
        }	  
        break;
	  
        case PLAYER_BROADCAST_CODE:   
#ifdef INCLUDE_BROADCAST
          // Create broadcast device as per normal
          if( deviceIO->local )
          {
            int argc = 0;
            char *argv[2];
            // Broadcast through the loopback device; note that this
            // wont work with distributed Stage.
            argv[argc++] = "addr";
            argv[argc++] = "127.255.255.255";
            deviceTable->AddDevice(deviceIO->player_id.port,
                                   deviceIO->player_id.type,
                                   deviceIO->player_id.index, 
                                   PLAYER_ALL_MODE, 
                                   new CBroadcastDevice(argc, argv));
	      
            // have we registered this port already?
            int j = 0;
            while( j<portcount )
            {
              if( portstmp[j] == deviceIO->player_id.port )
                break;
              j++;
            }
            if( j == portcount ) // we have not! 
              // we add this port to the array of ports we must listen on
              portstmp[portcount++] = deviceIO->player_id.port;
          }
#endif
          break;

          // devices not implemented
        case PLAYER_AUDIO_CODE:   
#ifdef VERBOSE
          printf( "Device type %d not yet implemented in Stage\n", 
                  deviceIO->player_id.type);
          fflush( stdout );
#endif
          break;
	  
        case 0:
#ifdef VERBOSE
          printf( "Player ignoring Stage device type %d\n", 
                  deviceIO->player_id.type);
          fflush( stdout );
#endif
          break;	  
	  
          // unknown device 
        default:
          printf( "Unknown device type %d for object ID (%d,%d,%d)\n", 			   
                  deviceIO->player_id.type, 
                  deviceIO->player_id.port, 
                  deviceIO->player_id.type, 
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
  assert( GlobalTime = (PlayerTime*)(new StageTime( clock, tfd )) );

  return( clock );
}

//#ifdef INCLUDE_BPS
  // DOUBLE-HACK -- now this won't work in stage, because we don't know to 
  //                which port to tie the device - BPG
  //
  // HACK -- Create BPS as per normal; it will use other simulated devices
  // The stage versus non-stage initialization needs some re-thinking.
  // ahoward
// deviceTable->AddDevice(global_playerport,PLAYER_BPS_CODE, 0, 
//                       PLAYER_READ_MODE, new CBpsDevice(0, NULL));
//#endif


#endif

/*
 * parses strings that look like "-laser:2"
 *   <str1> is the device string; <str2> is the argument string for the device
 *
 * if the string can be parsed, then the appropriate device will be created, 
 *    and 0 will be returned.
 * otherwise, -1 is retured
 */
int
parse_device_string(char* str1, char* str2)
{
  char* colon;
  uint16_t index = 0;
  CDevice* tmpdevice;

  // if we haven't initialized the PlayerTime pointer yet, do it now,
  // because it might be referenced by CDevice::PutData(), which can be
  // called by a device's constructor
  if(!GlobalTime)
    GlobalTime = (PlayerTime*)(new WallclockTime());

  if(!str1 || (str1[0] != '-') || (strlen(str1) < 2))
    return(-1);

  /* get the index */
  if((colon = strchr(str1,':')))
  {
    if(strlen(colon) < 2)
      return(-1);
    index = atoi(colon+1);
  }

  /* parse the config string into argc/argv format */
  int argc = 0;
  char* argv[32];
  char* tmpptr;
  if(str2 && strlen(str2))
  {
    argv[argc++] = strtok(str2, " ");
    while(argc < (int)sizeof(argv))
    {
      tmpptr = strtok(NULL, " ");
      if(tmpptr)
        argv[argc++] = tmpptr;
      else
        break;
    }
  }
  
  /* get the device name */
  if(!strncmp(PLAYER_MISC_STRING,str1+1,strlen(PLAYER_MISC_STRING)))
  {
#ifdef INCLUDE_MISC
    tmpdevice = new CMiscDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_MISC_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for misc device was not included at compile time.\n",stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_GRIPPER_STRING,str1+1,strlen(PLAYER_GRIPPER_STRING)))
  {
#ifdef INCLUDE_GRIPPER
    tmpdevice = new CGripperDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_GRIPPER_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for gripper device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_POSITION_STRING,str1+1,
                   strlen(PLAYER_POSITION_STRING)))
  {
#ifdef INCLUDE_POSITION
    tmpdevice = new CPositionDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_POSITION_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for position device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_SONAR_STRING,str1+1,strlen(PLAYER_SONAR_STRING)))
  {
#ifdef INCLUDE_SONAR
    tmpdevice = new CSonarDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_SONAR_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for sonar device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  // WARNING! the string compare is a bit fragile; must look for 'laserbeacon'
  // before looking for 'laser'...
  else if(!strncmp(PLAYER_LASERBEACON_STRING,str1+1,
                   strlen(PLAYER_LASERBEACON_STRING)))
  {
#ifdef INCLUDE_LASERBEACON
    tmpdevice = new CLaserBeaconDevice(argc,argv); 
    deviceTable->AddDevice(global_playerport,PLAYER_LASERBEACON_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for laserbeacon device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_LASER_STRING,str1+1,strlen(PLAYER_LASER_STRING)))
  {
#ifdef INCLUDE_LASER
    tmpdevice = new CLaserDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_LASER_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for laser device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_VISION_STRING,str1+1,strlen(PLAYER_VISION_STRING)))
  {
#ifdef INCLUDE_VISION
    tmpdevice = new CVisionDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_VISION_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for vision device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_PTZ_STRING,str1+1,strlen(PLAYER_PTZ_STRING)))
  {
#ifdef INCLUDE_PTZ
    tmpdevice = new CPtzDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_PTZ_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for ptz device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_AUDIO_STRING,str1+1,strlen(PLAYER_AUDIO_STRING)))
  {
#ifdef INCLUDE_AUDIO
    tmpdevice = new CAudioDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_AUDIO_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for audio device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_BROADCAST_STRING,str1+1,
                   strlen(PLAYER_BROADCAST_STRING)))
  {
#ifdef INCLUDE_BROADCAST
    tmpdevice = new CBroadcastDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_BROADCAST_CODE, index, 
                           PLAYER_ALL_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for broadcast device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_SPEECH_STRING,str1+1,strlen(PLAYER_SPEECH_STRING)))
  {
#ifdef INCLUDE_SPEECH
    tmpdevice = new CSpeechDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_SPEECH_CODE, index, 
                           PLAYER_WRITE_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for speech device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else if(!strncmp(PLAYER_GPS_STRING,str1+1,strlen(PLAYER_GPS_STRING)))
  {
    fputs("GPS device is not yet implemented in Player.\n",stderr);
    return(-1);
  }
  else if(!strncmp(PLAYER_BPS_STRING,str1+1,strlen(PLAYER_BPS_STRING)))
  {
#ifdef INCLUDE_BPS
    tmpdevice = new CBpsDevice(argc,argv);
    deviceTable->AddDevice(global_playerport,PLAYER_BPS_CODE, index, 
                           PLAYER_READ_MODE, tmpdevice);
#else
    fputs("\nWarning: Support for bps device was not included at compile time.\n",
          stderr);
    return(0);
#endif
  }
  else
  {
    printf("Error: unknown device \"%s\"\n", str1);
    return(-1);
  }
  return(0);
}

int
parse_config_file(char* fname)
{
  return(-1);
}

int main( int argc, char *argv[] )
{
  bool special_config = false;
  struct sockaddr_in listener;
  char auth_key[PLAYER_KEYLEN] = "";
  //struct sockaddr_in sender;
#ifdef PLAYER_LINUX
  socklen_t sender_len;
#else
  int sender_len;
#endif
  CClientData *clientData;
  //int player_sock = 0;
  
  // use these to keep track of many sockets in stage mode
  struct pollfd* ufds = 0;
  int* ports = 0;
  int num_ufds = 0;

  // make a copy of argv, so that strtok in parse_device_string
  // doesn't screw with it 
  char *new_argv[argc];
  for(int i=0;i<argc;i++)
    new_argv[i] = strdup(argv[i]);

  printf("** Player v%s ** ", PLAYER_VERSION);
  fflush(stdout);

  // parse args
  for( int i = 1; i < argc; i++ ) 
  {
    if(!strcmp(new_argv[i],"-stage"))
    {
#ifdef INCLUDE_STAGE
      if(++i<argc) 
      {
        strncpy(stage_io_directory, new_argv[i], sizeof(stage_io_directory));
        use_stage = true;
        printf("[Stage %s]", stage_io_directory );
      }
      else 
      {
        Usage();
        exit(-1);
      }
#else
      puts("\nSorry, Stage support was disabled at compile-time.");
      exit(-1);
#endif
    }
    else if(!strcmp(new_argv[i], "-key"))
    {
      if(++i<argc) 
      { 
        strncpy(auth_key,new_argv[i],sizeof(auth_key));
        // just in case...
        auth_key[sizeof(auth_key)-1] = '\0';
      }
      else 
      {
        Usage();
        exit(-1);
      }
    }
    else if(!strcmp(new_argv[i], "-gerkey"))
    {
      puts("Gerkey extensions enabled (good luck)....");
      player_gerkey = true;
    }
    else if(!strcmp(new_argv[i], "-dl"))
    {
      if(++i<argc) 
      { 
        void* handle;
        fprintf(stderr,"Opening shared object %s...", new_argv[i]);
        if(!(handle = dlopen(new_argv[i], RTLD_NOW)))
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
    }
    else if(!strcmp(new_argv[i], "-port"))
    {
      if(++i<argc) 
      { 
        global_playerport = atoi(new_argv[i]);
	    
        printf("[Port %d]", global_playerport);
      }
      else 
      {
        Usage();
        exit(-1);
      }
    }
    else if(!strcmp(new_argv[i], "-config"))
    {
      if(++i<argc)
      {
        if(parse_config_file(argv[i])<0)
          exit(-1);
        special_config = true;
      }
      else
      {
        Usage();
        exit(-1);
      }
    }
    else if(!strcmp(new_argv[i], "-sane"))
    {
      for(int i=0;i<ARRAYSIZE(sane_spec);i++)
      {
        if(parse_device_string(sane_spec[i],NULL) < 0)
        {
          fprintf(stderr, "Warning: got error while creating sane "
                  "device \"%s\"\n", sane_spec[i]);
        }
      }
      special_config = true;
    }
    else if((i+1)<argc && new_argv[i+1][0] != '-')
    {
      if(parse_device_string(new_argv[i],new_argv[i+1]) < 0)
      {
        Usage();
        exit(-1);
      }
      i++;
      special_config = true;
    }
    else
    {
      if(parse_device_string(new_argv[i],NULL) < 0)
      {
        Usage();
        exit(-1);
      }
      special_config = true;
    }
  }
  for(int i=0;i<argc;i++)
    free(new_argv[i]);

  // default behavior is to try to use the global config file; if that 
  // doesn't work use the sane spec
  if(!special_config && !use_stage)
  {
    if(parse_config_file(DEFAULT_PLAYER_CONFIGFILE)<0)
    {
      for(int i=0;i<ARRAYSIZE(sane_spec);i++)
      {
        if(parse_device_string(sane_spec[i],NULL) < 0)
        {
          fprintf(stderr, "Warning: got error while creating sane "
                  "device \"%s\"\n", sane_spec[i]);
        }
      }
    }
  }

  puts( "" ); // newline, flush

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
  
#ifdef INCLUDE_STAGE
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

#endif // INCLUDE_STAGE  

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

    //GlobalTime = (PlayerTime*)(new WallclockTime());
  }

  // create the client manager object.
  // it will start one reader thread and one writer thread and keep track
  // of all clients
  clientmanager = new ClientManager;

  // main loop: accept new connections and hand them off to the client
  // manager.
  for(;;) 
  {
    int num_connects;

    if((num_connects = poll(ufds,num_ufds,-1)) < 0)
    {
      if(errno == EINTR)
        continue;

      perror("poll() failed.");
      exit(-1);
    }

    if(!num_connects)
      continue;

    for(int i=0;i<num_ufds;i++)
    {
      if(ufds[i].revents & POLLIN)
      {
        clientData = new CClientData(auth_key,ports[i]);

        struct sockaddr_in cliaddr;
        sender_len = sizeof(cliaddr);
        memset( &cliaddr, 0, sizeof(cliaddr) );

        /* shouldn't block here */
        if((clientData->socket = accept(ufds[i].fd, 
                                        (struct sockaddr*)&cliaddr, 
                                        &sender_len)) == -1)
          //if((clientData->socket = accept(ufds[i].fd, NULL, 
          //                          &sender_len)) == -1)
        {
          perror("accept(2) failed: ");
          exit(-1);
        }

        // make the socket non-blocking
        if(fcntl(clientData->socket, F_SETFL, O_NONBLOCK) == -1)
        {
          perror("fcntl() failed while making socket non-blocking. quitting.");
          exit(-1);
        }

        /* got conn */
	
        // now reports the port number that was connected, instead of the 
        // global player port - RTV
	
        char clientIp[64];
        if( make_dotted_ip_address( clientIp, 64, 
                                    (uint32_t)(cliaddr.sin_addr.s_addr) ) )
        {
          // couldn't get the ip
          printf("** Player [port %d] client accepted on socket %d **\n",
                 ports[i],  clientData->socket);
        }
        else
          printf("** Player [port %d] client accepted from %s "
                 "on socket %d **\n", 
                 ports[i], clientIp, clientData->socket);
	
        /* add it to the manager's list */
        clientmanager->AddClient(clientData);
      }
    }
  }

  /* don't get here */
  return(0);
}



