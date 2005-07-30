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

#include <libplayercore/playercore.h>
#include <libplayertcp/playertcp.h>

#include "driverregistry.h"

/* getopt stuff */
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;
/* getopt stuff */

static int debuglevel = 1;
static int port = PLAYERTCP_DEFAULT_PORT;
static const char* cfgfilename;

void PrintCopyrightMsg();
void PrintUsage();
int ParseArgs(int argc, char** argv);

int
main(int argc, char** argv)
{
  PlayerTCP ptcp;
  ConfigFile cf;
  int debuglevel = 1;
  int port = PLAYERTCP_DEFAULT_PORT;

  register_drivers();

  if(ParseArgs(argc, argv) < 0)
  {
    PrintUsage();
    exit(-1);
  }

  ErrorInit(debuglevel);

  PrintCopyrightMsg();

  if(!cf.Load(cfgfilename))
  {
    PLAYER_ERROR1("failed to load config file %s", cfgfilename);
    exit(-1);
  }

  if(!cf.ParseAllDrivers())
  {
    PLAYER_ERROR1("failed to parse config file %s", cfgfilename);
    exit(-1);
  }

  if(ptcp.Listen(&port, 1) < 0)
  {
    PLAYER_ERROR1("failed to listen on port %d", port);
    exit(-1);
  }

  printf("Listening on port: %d\n", port);

  for(;;)
  {
    if(ptcp.Accept(-1) < 0)
    {
      PLAYER_ERROR("failed while accepting new connections");
      exit(-1);
    }
  }

  return(0);
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

  //PrintCopyrightMsg();

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
ParseArgs(int argc, char** argv)
{
  int ch;
  const char* optflags = "d:p:h";

  // Get letter options
  while((ch = getopt(argc, argv, optflags)) != -1)
  {
    switch (ch)
    {
      case 'd':
        debuglevel = atoi(optarg);
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case '?':
      case ':':
      case 'h':
      default:
        PrintUsage();
        exit(-1);
        break;
    }
  }

  argc -= optind;
  argv += optind;

  if(argc < 1)
    return(-1);
  
  cfgfilename = argv[0];

  return(0);
}
