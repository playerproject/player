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

#if HAVE_OPENCV
  #include <opencv/cv.h>
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
#include <math.h>

#include <replace/replace.h>
#include <libplayercore/playercore.h>

#include "socket_util.h" /* for create_and_bind_socket() */
#include "clientdata.h"
#include "clientmanager.h"
#include "timer.h"
#include "driverregistry.h"

// I can't find a way to do @prefix@ substitution in server/prefix.h that
// works in more than one version of autotools.  Fix this later.
//#include <prefix.h>

// Gazebo stuff
#if INCLUDE_GAZEBO
#include <gazebo.h>
#include "gz_client.h"
#include "gz_time.h"
#endif

// Log file stuff
#if INCLUDE_LOGFILE
#include "readlog_time.h"
#endif

//#define VERBOSE
//#define DEBUG

// default server update rate, in Hz
#define DEFAULT_SERVER_UPDATE_RATE 100.0

// true if the main loop should terminate
bool quit = false;

// true if sigint should be ignored
bool mask_sigint = false;

// keep track of the pointers to our various clients.
// that way we can cancel them at Shutdown
ClientManager* clientmanager = NULL;

// use this object to parse config files and command line args
ConfigFile configFile;

// for use in other places (cliendata.cc, for example)
char playerversion[] = VERSION;

bool autoassign_ports = false;
bool quiet_startup = false; // if true, minimize the console output on startup

int global_playerport = PLAYER_PORTNUM; // used to gen. useful output & debug

void
PrintCopyrightMsg()
{
  fprintf(stderr,"\n* Part of the Player/Stage/Gazebo Project [http://playerstage.sourceforge.net].\n");
  fprintf(stderr, "* Copyright (C) 2000 - 2005 Brian Gerkey, Richard Vaughan, Andrew Howard,\n* Nate Koenig, and contributors.");
  fprintf(stderr," Released under the GNU General Public License.\n");
  fprintf(stderr,"* Player comes with ABSOLUTELY NO WARRANTY.  This is free software, and you\n* are welcome to redistribute it under certain conditions; see COPYING\n* for details.\n\n");
}

/** @page commandline Command line options

The Player server is run as follows:

@verbatim
$ player [options] <configfile>
@endverbatim

where [options] is one or more of the following:

- -h             : Print usage message.
- -u &lt;rate&gt;: set server update rate, in Hz.
- -d &lt;level&gt;       : debug message level (0 = none, 1 = default, 9 = all).
- -t {tcp | udp} : transport protocol to use.  Default: tcp.
- -p &lt;port&gt;      : port where Player will listen. Default: 6665.
- -g &lt;id&gt;        : connect to Gazebo server with id &lt;id&gt; (an integer).
- -r &lt;logfile&gt;   : read data from &lt;logfile&gt; (readlog driver).
- -f &lt;speed&gt;     : readlog speed factor (e.g., 1 for normal speed, 2 for twice normal speed).
- -k &lt;key&gt;       : require client authentication with the given key.
- -q             : quiet startup mode: minimizes the console output on startup.

Note that only one of -s, -g and -r can be specified at any given time.

*/
void Usage()
{
  int maxlen=66;
  char** sortedlist;

  PrintCopyrightMsg();

  fprintf(stderr, "USAGE:  player [options] [<configfile>]\n\n");
  fprintf(stderr, "Where [options] can be:\n");
  fprintf(stderr, "  -h             : print this message.\n");
  fprintf(stderr, "  -u <rate>      : set server update rate to <rate> in Hz\n");
  fprintf(stderr, "  -d <level>     : debug message level (0 = none, 1 = default, 9 = all).\n");
  fprintf(stderr, "  -t {tcp | udp} : transport protocol to use.  Default: tcp\n");
  fprintf(stderr, "  -p <port>      : port where Player will listen. "
          "Default: %d\n", PLAYER_PORTNUM);
  fprintf(stderr, "  -g <path>      : connect to Gazebo instance at <path> \n");
  fprintf(stderr, "  -r <logfile>   : read data from <logfile> (readlog driver)\n");
  fprintf(stderr, "  -f <speed>     : readlog speed factor (e.g., 1 for normal speed, 2 for twice normal speed).\n");
  fprintf(stderr, "  -k <key>       : require client authentication with the "
          "given key\n");
  fprintf(stderr, "  -q             : quiet mode: minimizes the console output on startup.\n");
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
  fprintf(stderr, "\n\n");
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

/* setup some signal handlers */
void 
SetupSignalHandlers()
{
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


#ifdef HAVE_OPENCV
/* OpenCV error handler */
int CV_CDECL
cvErrorCallback(int status, const char* func_name,
                const char* err_msg, const char* file_name, int line, void *user)
{
  char msg[1024];
  snprintf(msg, sizeof(msg), "opencv %s:%d : %s", file_name, line, err_msg);
  PLAYER_ERROR1("%s", msg);
  return 0;
}
#endif

/* Trap errors from third-party libs */
void
SetupErrorHandlers()
{

// This needs some work.  If the build system has OpenCV installed, but
// the user disables all OpenCV-using drivers, then this bit of code is
// compiled, but the OpenCV libs aren't linked, and so the build fails.
#if 0
#ifdef HAVE_OPENCV
  // OpenCV handler
  cvRedirectError(cvErrorCallback);
#endif
#endif

  return;
}

/* for debugging */
void PrintHeader(player_msghdr_t hdr)
{
  printf("stx: %u\n", hdr.stx);
  printf("type: %u\n", hdr.type);
  printf("subtype: %u\n", hdr.subtype);
/*  printf("device: %u\n", hdr.device);
  printf("index: %u\n", hdr.device_index);
  printf("time: %u:%u\n", hdr.time_sec,hdr.time_usec);*/
  printf("times: %u:%u\n", hdr.timestamp_sec,hdr.timestamp_usec);
  printf("seq: %u\n", hdr.seq);
//  printf("conid: %u\n", hdr.conid);
  printf("size:%u\n", hdr.size);
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
  int i;
  for(i=0, device = deviceTable->GetFirstDevice(); device != NULL;
      i++, device = deviceTable->GetNextDevice(device))
  {
    int ret_temp=lookup_interface_code(device->addr.interface, &interface);
    assert(ret_temp == 0);
    
    if (device->driver != last_driver)
    {
      fprintf(stdout, "%d driver %s id %d:%s:%d\n",
              i, device->drivername,
              device->addr.robot, interface.name, device->addr.index);
    }
    else
    {
      fprintf(stdout, "%d        %*s id %d:%s:%d\n",
              i, strlen(device->drivername), "",
              device->addr.robot, interface.name, device->addr.index);
    }
    last_driver = device->driver;
  }

  printf("------------------------------------------------------------\n");
  return;
}



bool
ParseConfigFile(char* fname, int** ports, int* num_ports)
{
  char robotname[PLAYER_MAX_DEVICE_STRING_LEN];

  robotname[0]='\0';

  
  // parse the file
  if( !quiet_startup )
    printf("\nParsing configuration file \"%s\"\n", fname);
  
  if(!configFile.Load(fname))
    return(false);
  
  // a safe upper bound on the number of ports we'll need is the number of
  // entities in the config file (yes, i'm too lazy to dynamically
  // reallocate this buffer for a tighter bound).
  *ports = new int[configFile.GetSectionCount()];
  assert(*ports);
  // we'll increment this counter as we go
  *num_ports=0;

  // load each device specified in the file
  if(!configFile.ParseAllDrivers())
    return false;

  // Warn of any unused variables
  configFile.WarnUnused();

  // Print the device table
  if( !quiet_startup )
  {
    puts("Using device table:");
    PrintDeviceTable();
  }
      
  int i;
  Device *device;
  
  // Poll the device table for ports to monitor
  for (device = deviceTable->GetFirstDevice(); device != NULL;
       device = deviceTable->GetNextDevice(device))
  {
  	if (device->addr.robot == 0)
  	  device->addr.robot = global_playerport;
    // See if the port is already in the table
    for (i = 0; i < *num_ports; i++)
    {
      if ((*ports)[i] == device->addr.robot)
        break;
    }

    // If its not in the table, add it
    if (i >= *num_ports)
      (*ports)[(*num_ports)++] = device->addr.robot;
  }

  return(true);
}

// some drivers use libraries that need the arguments for initialization
int global_argc;
char** global_argv;

int main( int argc, char **argv)
{
  char auth_key[PLAYER_KEYLEN] = "";
  char *configfile = NULL;
  int gz_serverid = -1;
  char *gz_prefixid = NULL;
  char *readlog_filename = NULL;
  double readlog_speed = 1.0;
  double update_rate = DEFAULT_SERVER_UPDATE_RATE;
  int msg_level = 1;

  int *ports = NULL;
  struct pollfd *ufds = NULL;
  int num_ufds = 0;
  int protocol = PLAYER_TRANSPORT_TCP;

  printf("** Player v%s **", playerversion);
  fflush(stdout);

  global_argc = argc;
  global_argv = argv;

  // Register the available drivers in the driverTable; register_drivers()
  // is defined in driverregistry.cc
  register_drivers();
  
  // Trap ^C
  SetupSignalHandlers();

  // Trap errors from third-party libs
  SetupErrorHandlers();

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
    else if(!strcmp(argv[i],"-u"))
    {
      if(++i<argc)
      {
        update_rate = atof(argv[i]);
      }
      else
      {
        Usage();
        exit(-1);
      }
    }

    // Message level
    else if(!strcmp(argv[i],"-d"))
    {
      if(++i<argc)
      {
        msg_level = atoi(argv[i]);
      }
      else
      {
        Usage();
        exit(-1);
      }
    }
    // Gazebo support
    else if(!strcmp(argv[i], "-g"))
    {
      if(++i<argc)
      {
        gz_serverid = atoi(argv[i]);
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
    else if(!strcmp(argv[i], "-q"))
    {
      quiet_startup = true;
    }
    else
    {
      Usage();
      exit(-1);
    }
  }
  
  // by default print a copyright and license message
  if( !quiet_startup )
  {
    PrintCopyrightMsg();
    // then output a line of startup options, each in [square braces]
    printf( "Startup options:" );
    fflush(stdout);
  }
  
  printf(" [%s]", (protocol == PLAYER_TRANSPORT_TCP) ? "TCP" : "UDP");

  puts( "" ); // newline, flush

  // Initialize error handling
  ErrorInit(msg_level);

  if (gz_serverid >= 0)
  {
#ifdef INCLUDE_GAZEBO
    // Initialize gazebo client
    if (GzClient::Init(gz_serverid, gz_prefixid) != 0)
      exit(-1);

    // Use the clock from Gazebo
    if(GlobalTime)
      delete GlobalTime;
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
    ::ReadLog_filename = readlog_filename;
    ::ReadLog_speed = readlog_speed;
 
    // Use the clock from the log file
    if(GlobalTime)
      delete GlobalTime;
    GlobalTime = new ReadLogTime();
    assert(GlobalTime);
#else
    PLAYER_ERROR("Sorry, support for log files not included at compile-time.");
    exit(-1);
#endif
  }

  if(configfile != NULL)
  {
    // Parse the config file and instantiate drivers.  It builds up a list
    // of ports that we need to listen on.
    if(!ParseConfigFile(configfile, &ports, &num_ufds))
      exit(-1);
  }

  printf("Num of ports: %d\n", num_ufds);

  ufds = new struct pollfd[num_ufds];
  assert(ufds);

  for(int i=0;i<num_ufds;i++)
  {
    // setup the socket to listen on
    if((ufds[i].fd = create_and_bind_socket(1,ports[i],protocol,200)) == -1)
    {
      PLAYER_ERROR("create_and_bind_socket() failed; quitting");
      exit(-1);
    }
    ufds[i].events = POLLIN;

    if( !quiet_startup )
      printf("listening on port %d\n", ports[i]);
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

  delete[] ports;
  delete[] ufds;

  // Poll the device table for alwayson devices
  for(Device* device = deviceTable->GetFirstDevice(); 
      device;
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

    clientdata = new ClientDataTCP("",device->addr.robot);
    assert(clientdata);
        
    // to indicate that this one is a dummy
    clientdata->socket = -1;

    // add the dummy clients to the clientmanager
    clientmanager->AddClient(clientdata);

    req.code = device->addr.interface;
    req.index = device->addr.index;
    // TODO: should allow user to specify desired alwayson mode in the .cfg
    req.access = device->access;
    if(clientdata->UpdateRequested(req) != device->access)
    {
      PLAYER_ERROR1("Initial subscription failed for driver [%s]",
                    device->drivername);
      return(false);
    }
  }
  
  // check for empty device table
  if(!(deviceTable->Size()))
  {
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
  
  // compute seconds and nanoseconds from the given server update rate
  struct timespec ts;
  ts.tv_sec = (time_t)floor(1.0/update_rate);
  ts.tv_nsec = (long)floor(fmod(1.0/update_rate,1.0) * 1e9);
  
  // create and start the timer thread, which will periodically wake us
  // up to service clients
  Timer timer(clientmanager,ts);
  timer.Start();

  // main loop: keep updating the clientmanager until somebody says to stop
  while (!quit)
  {
    if(clientmanager->Update())
    {
      fputs("ClientManager::Update() errored; bailing.\n", stderr);
      exit(-1);
    }
  }

  // stop the timer thread
  timer.Stop();

  printf("** Player [port %d] quitting **\n", global_playerport );

#if INCLUDE_GAZEBO
  // Finalize gazebo client
  if (gz_serverid >= 0)
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

