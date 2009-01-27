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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

#ifndef _GLOBALS_H
#define _GLOBALS_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCORE_EXPORT
  #elif defined (playercore_EXPORTS)
    #define PLAYERCORE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCORE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCORE_EXPORT
#endif

#include <playerconfig.h>

class DeviceTable;
class PlayerTime;
class DriverTable;
class FileWatcher;
struct player_sd;

PLAYERCORE_EXPORT extern DeviceTable* deviceTable;
PLAYERCORE_EXPORT extern PlayerTime* GlobalTime;
PLAYERCORE_EXPORT extern DriverTable* driverTable;
PLAYERCORE_EXPORT extern FileWatcher* fileWatcher;
PLAYERCORE_EXPORT extern char playerversion[];
PLAYERCORE_EXPORT extern bool player_quit;
PLAYERCORE_EXPORT extern bool player_quiet_startup;

// global access to the command line arguments
PLAYERCORE_EXPORT extern int player_argc;
PLAYERCORE_EXPORT extern char** player_argv;

#if HAVE_PLAYERSD
PLAYERCORE_EXPORT extern struct player_sd* globalSD;
#endif

PLAYERCORE_EXPORT void player_globals_init(void);
PLAYERCORE_EXPORT void player_globals_fini();

#endif
