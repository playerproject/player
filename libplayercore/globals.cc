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

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <libplayercore/globals.h>
#include <libplayercore/devicetable.h>
#include <libplayercore/drivertable.h>
#include <libplayercore/filewatcher.h>
#include <libplayercore/playertime.h>
#include <libplayercore/wallclocktime.h>

#if HAVE_PLAYERSD
  #include <libplayersd/playersd.h>
#endif

// this table holds all the currently *instantiated* devices
PLAYERCORE_EXPORT DeviceTable* deviceTable;

// this table holds all the currently *available* drivers
PLAYERCORE_EXPORT DriverTable* driverTable;

// the global PlayerTime object has a method
//   int GetTime(struct timeval*)
// which everyone must use to get the current time
PLAYERCORE_EXPORT PlayerTime* GlobalTime;

// global class for watching for changes in files and sockets
PLAYERCORE_EXPORT FileWatcher* fileWatcher;

PLAYERCORE_EXPORT char playerversion[32];

PLAYERCORE_EXPORT bool player_quit;
PLAYERCORE_EXPORT bool player_quiet_startup;

// global access to the cmdline arguments
int player_argc;
char** player_argv;

#if HAVE_PLAYERSD
struct player_sd* globalSD;
#endif

void
player_globals_init()
{
  deviceTable = new DeviceTable();
  driverTable = new DriverTable();
  GlobalTime = new WallclockTime();
  fileWatcher = new FileWatcher();
  strncpy(playerversion, PLAYER_VERSION, sizeof(playerversion));
  player_quit = false;
  player_quiet_startup = false;
#if HAVE_PLAYERSD
  globalSD = player_sd_init();
#endif
}

void
player_globals_fini()
{
  delete deviceTable;
  delete driverTable;
  delete GlobalTime;
  delete fileWatcher;
#if HAVE_PLAYERSD
  if(globalSD)
    player_sd_fini(globalSD);
#endif
}
