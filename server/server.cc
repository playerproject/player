/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005 -
 *     Brian Gerkey
 *
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/** @ingroup utils */
/** @{ */
/** @defgroup util_player player server
 @brief TCP server that allows remote access to Player devices.

The most commonly-used of the Player utilities, @b player is a TCP
server that allows remote access to devices.  It is normally executed
on-board a robot, and is given a configuration file that maps the robot's
hardware to Player devices, which are then accessible to client programs.
The situation is essentially the same when running a simulation.

@section Usage

@code
player [-q] [-d <level>] [-p <port>] [-h] <cfgfile>
@endcode
Arguments:
- -h : Give help info; also lists drivers that were compiled into the server.
- -q : Quiet startup
- -d \<level\> : Set the debug level, 0-9.  0 is only errors, 1 is errors +
warnings, 2 is errors + warnings + diagnostics, higher levels may give
more output.  Default: 1.
- -p \<port\> : Establish the default TCP port, which will be assigned to
any devices in the configuration file without an explicit port assignment.
Default: 6665.
- -l \<logfile\>: File to log messages to (default stdout only)
- \<cfgfile\> : The configuration file to read.

@section Example

@code
$ player pioneer.cfg

* Part of the Player/Stage/Gazebo Project
* [http://playerstage.sourceforge.net].
* Copyright (C) 2000 - 2005 Brian Gerkey, Richard Vaughan, Andrew Howard,
* Nate Koenig, and contributors. Released under the GNU General Public
* License.
* Player comes with ABSOLUTELY NO WARRANTY.  This is free software, and you
* are welcome to redistribute it under certain conditions; see COPYING
* for details.

Listening on ports: 6665
@endcode

*/
/** @} */

#include <config.h>

#include <libplayercore/playercore.h>

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#if !defined (WIN32) || defined (__MINGW32__)
  #include <unistd.h>
#endif

// Includes for umask() and open
#ifdef PLAYER_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

#include <stdexcept>
using std::runtime_error;
#include <string>
using std::string;

#include <libplayertcp/playertcp.h>
#include <libplayertcp/playerudp.h>
#include <libplayerinterface/functiontable.h>
#include "libplayerdrivers/driverregistry.h"
#include <config.h>

#if HAVE_PLAYERSD
  #include <libplayersd/playersd.h>
#endif

#if !HAVE_GETOPT
  #include <replace.h>
#endif

void PrintVersion();
void PrintCopyrightMsg();
void PrintUsage();
int ParseArgs(int* port, int* debuglevel,
              char** cfgfilename, int* gz_serverid, char** logfilename,
              bool &shoud_daemonize,
              int argc, char** argv);
void Quit(int signum);
void Cleanup();

int lockfile_id = -1;
bool process_is_daemon = false;

#ifdef PLAYER_UNIX
runtime_error posix_exception(const string &prefix);
bool daemonize_self();
#endif

PlayerTCP* ptcp;
PlayerUDP* pudp;
ConfigFile* cf;

int
main(int argc, char** argv)
{
  // provide global access to the cmdline args
  player_argc = argc;
  player_argv = argv;

  int debuglevel = 1;
  int port = PLAYERTCP_DEFAULT_PORT;
  int gz_serverid = -1;
  int* ports = NULL;
  int* new_ports = NULL;
  int num_ports = 0;
  char* cfgfilename = NULL;
  char * logfileName = NULL;

#ifdef WIN32
  if(signal(SIGINT, Quit) == SIG_ERR)
  {
    PLAYER_ERROR1("signal() failed: %s", strerror(errno));
    exit(-1);
  }
#else
  struct sigaction quit_action = {{0}};
  quit_action.sa_handler = Quit;
  sigemptyset (&quit_action.sa_mask);
  quit_action.sa_flags = SA_RESETHAND;
  struct sigaction ignore_action = {{0}};
  ignore_action.sa_handler = SIG_IGN;
  sigemptyset (&ignore_action.sa_mask);
  if(sigaction(SIGINT, &quit_action, NULL) != 0)
  {
    PLAYER_ERROR1("signal() failed: %s", strerror(errno));
    exit(-1);
  }
  if(sigaction(SIGTERM, &quit_action, NULL) != 0)
  {
    PLAYER_ERROR1("signal() failed: %s", strerror(errno));
    exit(-1);
  }
  if(sigaction(SIGPIPE, &ignore_action, NULL) != 0)
  {
    PLAYER_ERROR1("signal() failed: %s", strerror(errno));
    exit(-1);
  }
#endif

  player_globals_init();
  player_register_drivers();
  playerxdr_ftable_init();
  itable_init();

  ptcp = new PlayerTCP();
  assert(ptcp);

  pudp = new PlayerUDP();
  assert(pudp);

  PrintVersion();

  bool should_daemonize = false;

  char *logfilename_unres = NULL;
  char *cfgfilename_unres = NULL;

  if(ParseArgs(&port, &debuglevel, &cfgfilename_unres, &gz_serverid,
               &logfilename_unres, should_daemonize, argc, argv) < 0)
  {
    PrintUsage();
    exit(-1);
  }

#ifdef PLAYER_UNIX
  // Adjust logfileName and cfgfilename to be absolute paths
  try
  {
      if(logfilename_unres != NULL
         && (logfileName = realpath(logfilename_unres, logfileName)) == NULL)
      {
          throw posix_exception("Call to realpath on supplied"
                                " log file name failed: ");
      }
      
      if((cfgfilename = realpath(cfgfilename_unres, cfgfilename)) == NULL)
      {
          throw posix_exception("Call to realpath on supplied config file name"
                                " failed: ");
      }
  }
  catch(runtime_error &re)
  {
      PLAYER_ERROR1("Error while processing arguments: %s", re.what());
      Cleanup();
      return 1;
  }

  if(should_daemonize)
  {
      
      // Alert the user that we're daemonizing
      printf("Forking to daemon process...\n");

      try
      {
          process_is_daemon = daemonize_self();
      }
      catch(runtime_error &re)
      {
          PLAYER_ERROR1("Error while daemonizing: %s", re.what());
          Cleanup();
          return 1;
      }

      // Die if we're not the daemon process
      if(! process_is_daemon)
      {
          Cleanup();
          return 0;
      }
  }
#else

  // Don't do things for daemonization
  logfileName = logfilename_unres;
  cfgfilename = cfgfilename_unres;

  if(should_daemonize)
  {
      fprintf(stderr, "Cannot daemonize on a non-posix system");
      Cleanup();
      exit(1);
  }
#endif

  FILE* logfile = NULL;
  if (logfileName)
    logfile = fopen(logfileName,"a");
  ErrorInit(debuglevel,logfile);

  PrintCopyrightMsg();

  cf = new ConfigFile("localhost",port);
  assert(cf);

  if(!cf->Load(cfgfilename))
  {
    PLAYER_ERROR1("failed to load config file %s", cfgfilename);
    exit(-1);
  }

  if(!cf->ParseAllInterfaces())
  {
    PLAYER_ERROR1("failed to parse config file %s interface blocks", cfgfilename);
    exit(-1);
  }

  if(!cf->ParseAllDrivers())
  {
    PLAYER_ERROR1("failed to parse config file %s driver blocks", cfgfilename);
    exit(-1);
  }

  cf->WarnUnused();

  if (deviceTable->Size() == 0)
  {
    PLAYER_ERROR("No devices read in configuration file. Is it correct?\nExiting...");
    exit(-1);
  }

  // Collect the list of ports on which we should listen
  ports = (int*)calloc(deviceTable->Size(),sizeof(int));
  assert(ports);
  new_ports = (int*)calloc(deviceTable->Size(),sizeof(int));
  assert(new_ports);
  num_ports = 0;
  for(Device* device = deviceTable->GetFirstDevice();
      device;
      device = deviceTable->GetNextDevice(device))
  {
    // don't listen locally for remote devices
    if(!strcmp(device->drivername,"remote"))
      continue;
    int i;
    for(i=0;i<num_ports;i++)
    {
      if((int)device->addr.robot == ports[i])
        break;
    }
    if(i==num_ports)
      ports[num_ports++] = device->addr.robot;
  }

  if(ptcp->Listen(ports, num_ports, new_ports) < 0)
  {
    PLAYER_ERROR("failed to listen on requested TCP ports");
    Cleanup();
    exit(-1);
  }

  if(pudp->Listen(ports, num_ports) < 0)
  {
    PLAYER_ERROR("failed to listen on requested UDP ports");
    Cleanup();
    exit(-1);
  }

  // Go back through and relabel the devices for which ports got
  // auto-assigned during Listen().
  // TODO: currently this only works for port=0.  Should add support to
  // specify multiple auto-assigned ports (e.g., with unique negative numbers).
  for(int i=0;i<num_ports;i++)
  {
    // Get the old port and the new port
    unsigned int oport = ports[i];
    unsigned int nport = new_ports[i];

    // If the old and new ports are the same, nothing to do
    if(oport == nport)
      continue;

    // Iterate the device table, looking for devices with the old port
    for(Device* device = deviceTable->GetFirstDevice();
        device;
        device = deviceTable->GetNextDevice(device))
    {
      if(device->addr.robot == oport)
        device->addr.robot = nport;
    }
  }

#if HAVE_PLAYERSD
  char servicename[PLAYER_SD_NAME_MAXLEN];
  char host[PLAYER_SD_NAME_MAXLEN];
  if(gethostname(host,sizeof(host)) == -1)
    strcpy(host,"localhost");

  // Register all devices with zerconf
  int zcnt=0;
  for(Device* device = deviceTable->GetFirstDevice();
      device;
      device = deviceTable->GetNextDevice(device))
  {
    snprintf(servicename,sizeof(servicename),"%s %s:%u",
             host, interf_to_str(device->addr.interf), device->addr.index);
    if(player_sd_register(globalSD,servicename,device->addr) != 0)
    {
      PLAYER_WARN("player_sd_register returned error");
    }
    else
      zcnt++;
  }
  printf("registered %d devices\n", zcnt);
#endif

  printf("Listening on ports: ");
  for(int i=0;i<num_ports;i++)
    printf("%d ", new_ports[i]);
  puts("");

  free(ports);
  free(new_ports);

  if(deviceTable->StartAlwaysonDrivers() != 0)
  {
    PLAYER_ERROR("failed to start alwayson drivers");
    Cleanup();
    exit(-1);
  }
 
  while(!player_quit)
  {
    // wait until something other than driver requested watches happens
    int numready = fileWatcher->Wait(0.01); // run at a minimum of 100Hz for other drivers
    if (numready > 0)
    {
      if(ptcp->Accept(0) < 0)
      {
        PLAYER_ERROR("failed while accepting new TCP connections");
        break;
      }

      if(ptcp->Read(0,false) < 0)
      {
        PLAYER_ERROR("failed while reading from TCP clients");
        break;
      }

      if(pudp->Read(0) < 0)
      {
        PLAYER_ERROR("failed while reading from UDP clients");
        break;
      }
    }
    deviceTable->UpdateDevices();

    if(ptcp->Write(false) < 0)
    {
      PLAYER_ERROR("failed while writing to TCP clients");
      break;
    }

    if(pudp->Write() < 0)
    {
      PLAYER_ERROR("failed while writing to UDP clients");
      break;
    }
  }

  puts("Quitting.");

  Cleanup();

  return(0);
}

void
Cleanup()
{
  delete ptcp;
  delete pudp;

  if(deviceTable->StopAlwaysonDrivers() != 0)
  {
    PLAYER_ERROR("failed to stop alwayson drivers");
  }
#if PLAYER_UNIX
  // If we are a daemon, close all open file descriptors (this also unlocks the
  // lockfile)
  if(process_is_daemon)
  {
      for(int fd = sysconf(_SC_OPEN_MAX) - 1; fd <= 0; --fd)
      {
          close(fd);
      }
  }
#endif
  player_globals_fini();
  delete cf;
}

void
Quit(int signum)
{
  switch(signum)
    {
#if PLAYER_UNIX
    case SIGHUP:
        break;
#endif
    case SIGTERM:
    default:
        player_quit = true;
        break;
    }
}

void
PrintVersion()
{
  fprintf(stderr, "Player v.%s\n", PLAYER_VERSION);
}

void
PrintCopyrightMsg()
{
  fprintf(stderr,"\n* Part of the Player/Stage/Gazebo Project [http://playerstage.sourceforge.net].\n");
  fprintf(stderr, "* Copyright (C) 2000 - 2013 Brian Gerkey, Richard Vaughan, Andrew Howard,\n* Nate Koenig, and contributors.");
  fprintf(stderr," Released under the GNU General Public License.\n");
  fprintf(stderr,"* Player comes with ABSOLUTELY NO WARRANTY.  This is free software, and you\n* are welcome to redistribute it under certain conditions; see COPYING\n* for details.\n\n");
}

void
PrintUsage()
{
  int maxlen=66;
  char** sortedlist;

  fprintf(stderr, "USAGE:  player [options] [<configfile>]\n\n");
  fprintf(stderr, "Where [options] can be:\n");
  fprintf(stderr, "  -h             : print this message.\n");
  fprintf(stderr, "  -d <level>     : debug message level (0 = none, 1 = default, 9 = all).\n");
  fprintf(stderr, "  -p <port>      : port where Player will listen. "
          "Default: %d\n", PLAYERTCP_DEFAULT_PORT);
  fprintf(stderr, "  -q             : quiet mode: minimizes the console output on startup.\n");
  fprintf(stderr, "  -l <logfile>   : log player output to the specified file\n");
  fprintf(stderr, "  -s             : fork to a daemon process as the current user.\n");
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


int
ParseArgs(int* port, int* debuglevel, char** cfgfilename, int* gz_serverid,
          char **logfilename, bool &should_daemonize,
          int argc, char** argv)
{
  int ch;
  const char* optflags = "d:p:l:hqs";

  // Get letter options
  while((ch = getopt(argc, argv, optflags)) != -1)
  {
    switch (ch)
    {
      case 'q':
        player_quiet_startup = true;
        break;
      case 'd':
        *debuglevel = atoi(optarg);
        break;
      case 'l':
        *logfilename = optarg;
        break;
      case 'p':
        *port = atoi(optarg);
        break;
      case 's':
        should_daemonize = true;
        break;
      case '?':
      case ':':
      case 'h':
      default:
        return(-1);
    }
  }

  if(optind >= argc)
    return(-1);

  *cfgfilename = argv[optind];

  return(0);
}

#ifdef PLAYER_UNIX
// Convenience function to issue an execption containing th output of strerror
runtime_error posix_exception(const string &prefix)
{
    int errno_save = errno;
    errno = 0;

    return runtime_error(prefix + strerror(errno_save));
}
#endif

#ifdef PLAYER_UNIX
// Turn the server into a daemon.
// Adapted from Levent Karakas' daemon example:
// http://www.enderunix.org/docs/eng/daemon.php
//
// Returns false for the parent process, true for the daemon process
bool daemonize_self()
{
    
    // Check if we're already a daemon
    if(getppid() == 1)
        return true;

    // fork daemon to remove ourselves from any shell to which we may be subject
    {
        int forkval = -1;
        if((forkval = fork()) < 0)
        {
            // Fork error
            throw posix_exception("Error in daemonize_self:fork(): ");
        }
        else if(forkval > 0)
        {
            // We are the parent process, and should die
            return false;
        }
    }

    // Set ourselves as the process group leader
    if(setsid() < 0)
    {
        // setsid error
        throw posix_exception("Error in daemonize_self:setsid(): ");
    }

    // Close all open file descriptors
    for(int fd = sysconf(_SC_OPEN_MAX) - 1; fd <= 0; --fd)
    {
        close(fd);
    }

    // Change directory to the tmp directory in case some drivers decide to drop
    // files in the current directory.
    if(chdir("/tmp") < 0)
    {
        // Chdir error
        throw posix_exception("Error in daemonize(): chdir(): ");
    }

    // Set permissions for newly created files
    umask(027);

    // Direct stdin from /dev/null
    if(open("/dev/null", O_RDWR) < 0)
    {
        throw posix_exception("Error in daemonize_self: open(/dev/null): ");
    }

    // Direct stdout to temp file in working directory
    if(open("/tmp/player.stdout", O_RDWR | O_CREAT, 0640) < 0)
    {
        throw posix_exception("Error in daemonize_self: open stdout: ");
    }

    // Direct stderr to temp file in working directory
    if(open("/tmp/player.stderr", O_RDWR | O_CREAT, 0640) < 0)
    {
        throw posix_exception("Error in daemonize_self: open stderr: ");
    }

    // Open a lock file    
    lockfile_id = open("/tmp/player.lock", O_RDWR | O_CREAT, 0640);
    if(lockfile_id < 0)
    {
        throw posix_exception("Error in daemonize_self: open lockfile: ");
    }

    // Lock the lockfile
    if(lockf(lockfile_id, F_TLOCK, 0) < 0)
    {
        // Could not lock lockfile.  There is probably already a player instance
        // running, and we should exit.
        throw posix_exception("Error in daemonize_self: lock lockfile: ");
    }
        
    // Write our pid to the lockfile
    char str [10];
    if(snprintf(str, 10, "%d\n", getpid()) < 0)
    {
        // Error in snprintf
        throw posix_exception("Error in daemonize: stringize pid: ");
    }
    if(write(lockfile_id, str, strlen(str)) < 0)
    {
        // Error in write
        throw posix_exception("Error in daemonize: write pid: ");
    }

    // Setup our signal mask
    struct sigaction signal_action = {{0}};

    // Handle SIGHUP and SIGTERM
    signal_action.sa_handler  = &Quit;

    if(sigaction(SIGHUP, &signal_action, NULL) < 0)
    {
        throw posix_exception("Error in daemonize: set SIGHUP action: ");
    }
    if(sigaction(SIGTERM, &signal_action, NULL) < 0)
    {
        throw posix_exception("Error in daemonize: set SIGTERM action: ");
    }

    // Ignore SIGCHILD, SIGSTP, SIGTTOU, SIGTTIN (tty signals)
    signal_action.sa_handler = SIG_IGN;

    if(sigaction(SIGCHLD, &signal_action, NULL) < 0)
    {
        throw posix_exception("Error in daemonize: ignore SIGCHILD: ");
    }

    if(sigaction(SIGTSTP, &signal_action, NULL) < 0)
    {
        throw posix_exception("Error in daemonize: ignore SIGSTP: ");
    }

    if(sigaction(SIGTTOU, &signal_action, NULL) < 0)
    {
        throw posix_exception("Error in daemonize: ignore SIGTTOU: ");
    }

    if(sigaction(SIGTTIN, &signal_action, NULL) < 0)
    {
        throw posix_exception("Error in daemonize: ignore SIGTTIN: ");
    }

    return true;
}
#endif
