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
 *   the P2 vision device.  it takes pan tilt zoom commands for the
 *   sony PTZ camera (if equipped), and returns color blob data gathered
 *   from ACTS, which this device spawns and then talks to.
 */

#ifndef ARENAVISIONDEVICE
#define ARENAVISIONDEVICE

#include <pthread.h>

#include "arenalock.h"
#include "visiondevice.h"

class CArenaVisionDevice:public CVisionDevice 
{
  private:
    // RTV - access to lock through GetLock()
    CArenaLock alock;
    // !RTV

  public:
    CArenaVisionDevice(int num, char* configfile, bool oldacts);

    ~CArenaVisionDevice();
   
    virtual CArenaLock* GetLock( void ){ return &alock; };
    
    virtual int Setup();
    virtual int Shutdown();
    
    // override these to do nothing.
    virtual void GetCommand(unsigned char *, size_t maxsize){ /*empty*/ };
    virtual void PutCommand(unsigned char *, size_t maxsize){ /*empty*/ };
};

#endif

