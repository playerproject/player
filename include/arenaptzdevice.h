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
 * the interface to arena's emulation of the Sony EVI-D30 PTZ camera device
 */

#ifndef ARENAPTZDEVICE
#define ARENAPTZDEVICE
#include <pthread.h>
#include <unistd.h>

#include "arenalock.h"
#include "ptzdevice.h"

class CArenaPtzDevice:public CPtzDevice {
 private:
   CArenaLock alock;

 public:
  CArenaPtzDevice(char *port);
  ~CArenaPtzDevice();

  // this is a very simple device - we just override the setup/shutdown
  // behaviour of the real ptz device
  virtual int Setup();
  virtual int Shutdown();

  virtual CArenaLock* GetLock( void ){ return &alock; };

};

#endif
