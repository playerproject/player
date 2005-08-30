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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include <libplayercore/playercore.h>
#include <libplayertcp/playertcp.h>
#include <libplayerxdr/functiontable.h>
#include <libplayerdrivers/driverregistry.h>

/****************/
/* getopt stuff */
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;
/* getopt stuff */
/****************/

// These are declared in libplayercore/globals.cc
extern bool player_quit;
extern bool player_quiet_startup;

void PrintCopyrightMsg();
void PrintUsage();
int ParseArgs(int* port, int* debuglevel, const char** cfgfilename, 
              int argc, char** argv);
void Quit(int signum);
void Cleanup();

int
main(int argc, char** argv)
{
  int debuglevel = 1;
  int port = PLAYERTCP_DEFAULT_PORT;
  int* ports;
  int num_ports;
  const char* cfgfilename;
  PlayerTCP ptcp;
  ConfigFile* cf;

  if(signal(SIGINT, Quit) == SIG_ERR)
  {
    PLAYER_ERROR1("signal() failed: %s", strerror(errno));
    exit(-1);
  }

  if(signal(SIGPIPE, SIG_IGN) == SIG_ERR)
  {
    PLAYER_ERROR1("signal() failed: %s", strerror(errno));
    exit(-1);
  }

  player_register_drivers();
  playerxdr_ftable_init();

  if(ParseArgs(&port, &debuglevel, &cfgfilename, argc, argv) < 0)
  {
    PrintUsage();
    exit(-1);
  }

  ErrorInit(debuglevel);

  PrintCopyrightMsg();

  cf = new ConfigFile("localhost",port);
  assert(cf);

  if(!cf->Load(cfgfilename))
  {
    PLAYER_ERROR1("failed to load config file %s", cfgfilename);
    exit(-1);
  }

  if(!cf->ParseAllDrivers())
  {
    PLAYER_ERROR1("failed to parse config file %s", cfgfilename);
    exit(-1);
  }

  if(deviceTable->StartAlwaysonDrivers() != 0)
  {
    PLAYER_ERROR("failed to start alwayson drivers");
    Cleanup();
    exit(-1);
  }

  // Collect the list of ports on which we should listen
  ports = (int*)calloc(deviceTable->Size(),sizeof(int));
  assert(ports);
  num_ports = 0;
  for(Device* device = deviceTable->GetFirstDevice();
      device;
      device = deviceTable->GetNextDevice(device))
  {
    int i;
    for(i=0;i<num_ports;i++)
    {
      if((int)device->addr.robot == ports[i])
        break;
    }
    if(i==num_ports)
      ports[num_ports++] = device->addr.robot;
  }

  if(ptcp.Listen(ports, num_ports) < 0)
  {
    PLAYER_ERROR("failed to listen on requested ports");
    Cleanup();
    exit(-1);
  }

  printf("Listening on ports: ");
  for(int i=0;i<num_ports;i++)
    printf("%d ", ports[i]);
  puts("");

  free(ports);

  while(!player_quit)
  {
    if(ptcp.Accept(0) < 0)
    {
      PLAYER_ERROR("failed while accepting new connections");
      break;
    }
    if(ptcp.Read(1) < 0)
    {
      PLAYER_ERROR("failed while reading");
      break;
    }
    deviceTable->UpdateDevices();
    if(ptcp.Write() < 0)
    {
      PLAYER_ERROR("failed while reading");
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
  delete deviceTable;
  delete driverTable;
}

void
Quit(int signum)
{
  player_quit = true;
}

void
PrintCopyrightMsg()
{
  fprintf(stderr,"\n* Part of the Player/Stage/Gazebo Project [http://playerstage.sourceforge.net].\n");
  fprintf(stderr, "* Copyright (C) 2000 - 2005 Brian Gerkey, Richard Vaughan, Andrew Howard,\n* Nate Koenig, and contributors.");
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
  fprintf(stderr, "  -u <rate>      : set server update rate to <rate> in Hz\n");
  fprintf(stderr, "  -d <level>     : debug message level (0 = none, 1 = default, 9 = all).\n");
  fprintf(stderr, "  -t {tcp | udp} : transport protocol to use.  Default: tcp\n");
  fprintf(stderr, "  -p <port>      : port where Player will listen. "
          "Default: %d\n", PLAYERTCP_DEFAULT_PORT);
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

int 
ParseArgs(int* port, int* debuglevel, const char** cfgfilename, 
          int argc, char** argv)
{
  int ch;
  const char* optflags = "d:p:hq";

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
      case 'p':
        *port = atoi(optarg);
        break;
      case '?':
      case ':':
      case 'h':
      default:
        return(-1);
    }
  }

  argc -= optind;
  argv += optind;

  if(argc < 1)
    return(-1);
  
  *cfgfilename = argv[0];

  return(0);
}
