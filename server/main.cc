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

#if HAVE_LIB_DL
  #include <dlfcn.h>
#endif

#include <sys/types.h>  /* for accept(2) */
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <string.h> // for bzero()
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
#include <stagetime.h>
CDevice* StageDeviceInit();
#endif

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
  fprintf(stderr, "USAGE: player [-p <port>] [-s] [-dl <shlib>] "
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

bool
parse_config_file(char* fname)
{
  DriverEntry* entry;
  CDevice* tmpdevice;
  char interface[PLAYER_MAX_DEVICE_STRING_LEN];
  char* colon;
  int index;
  int robot_id=0;
  char robot_id_string[PLAYER_MAX_DEVICE_STRING_LEN];
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
    
    // look for a leading robot id and colon
    if((colon = strchr(interface,':')) && 
       ((strlen(interface) - strlen(colon)) >= 1) &&
       isdigit(interface[0]))
    {
      // get the robot id
      strncpy(robot_id_string,interface,strlen(interface)-strlen(colon));
      robot_id_string[strlen(interface)-strlen(colon)] = '\0';
      robot_id = atoi(robot_id_string);
      memmove(interface,colon+1,strlen(colon));
    }
    //else
    //robot_id = 0;

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

    printf("  loading driver \"%s\" as device \"%d:%s:%d\"\n", 
           driver, robot_id, interface, index);

    player_device_id_t id;
    id.code = code;
    id.robot = robot_id;
    id.index = index;

    /* look for the indicated driver in the available device table */
    if(!(entry = driverTable->GetDriverEntry(driver)))
      {
        PLAYER_ERROR1("Couldn't find driver \"%s\"", driver);
        return(false);
      }
    
    if(!(tmpdevice = (*(entry->initfunc))(interface,&configFile,i)))
      {
        PLAYER_ERROR2("Initialization failed for driver \"%s\" as interface \"%s\"\n",
                      driver, interface);
        exit(-1);
      }
    // add this device into the table.  subtract one from the parent id,
    // because the entities are indexed from 1 (the "root" entity is 0)
    // and the devicetable is indexed from 0.
    deviceTable->AddDevice(id, driver, entry->access, tmpdevice,
			   configFile.entities[i].parent-1);
    
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
  
  configFile.WarnUnused();

  puts("Done.");

  return(true);
}

int CDevice::argc = 0;
char** CDevice::argv = NULL;

int main( int argc, char *argv[] )
{
  // store the command line in static members where all devices can see it
  CDevice::argc = argc;
  CDevice::argv = argv;

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

      printf( "Copyright (C) 1999-2003 \n"
	      "  Brian Gerkey    <gerkey@usc.edu>\n"
	      "  Andrew Howard   <ahoward@usc.edu>\n"
	      "  Richard Vaughan <vaughan@hrl.com>\n"
	      "  and contributors.\n\n"
	      
	      "Part of the Player/Stage Project "
	      "[http://playerstage.sourceforge.net]\n"
	      
	      // GNU disclaimer
	      "This is free software; see the source for copying conditions.  "
	      "There is NO\nwarranty; not even for MERCHANTABILITY or "
	      "FITNESS FOR A PARTICULAR PURPOSE.\n\n" );
      exit(0);
    }
    else if(!strcmp(argv[i],"-s"))
    {
#if INCLUDE_STAGE
      printf( "[stage mode]" );
      use_stage = true;
      
      player_device_id_t id;
      id.code = PLAYER_STAGE_CODE;
      id.robot = 0;
      id.index = 0;
    
      CDevice* stagedevice = StageDeviceInit();
      // add this device into the table. 
      deviceTable->AddDevice( id, "stage", 'a', stagedevice, 1);
      
      if(stagedevice->Subscribe(NULL))
	{
	  PLAYER_ERROR("Initial subscription failed to StageDevice\n"),
          exit(-1);
	} 
#else
      printf( "Sorry, the Stage simulator was not included at compile-time.\n" );
      exit(-1);
#endif
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
    else if(i == (argc-1) && !use_stage )
    {
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
  
  // check for empty device table
  if(!(deviceTable->Size()))
    {
      if( use_stage )
	PLAYER_ERROR("No Stage device instantied");
      else
	PLAYER_WARN("No devices instantiated; perhaps you should supply " 
		    "a configuration file?");
    }

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
    
  // create the client manager object.
  clientmanager = new ClientManager(ufds,ports,num_ufds,auth_key);

  // main loop: update the clientmanager 
  // (which calls poll(2) so this isn't a busy-loop)
  for(;;) 
  {
    if(clientmanager->Update())
    {
      fputs("ClientManager::Update() errored; bailing.\n", stderr);
      exit(-1);
    }
  }

  /* don't get here */
  return(0);
}



