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

#ifndef VISIONDEVICE
#define VISIONDEVICE

#include <pthread.h>

#include "lock.h"
#include "device.h"

class CVisionDevice:public CDevice 
{
  private:
    pthread_t thread;   // the thread that continuously reads from ACTS
    int debuglevel;             // debuglevel 0=none, 1=basic, 2=everything
    int pid;      // ACTS's pid so we can kill it later

    // RTV - access to lock through GetLock()
    CLock lock;
    // !RTV

  public:
    bool useoldacts;    // whether or not we use old ACTS
    int sock;               // laser device file descriptor
    int portnum;  // port number where we'll run ACTS
    char configfilepath[VISION_CONFIGFILE_NAME_SIZE];  // path to configfile
    unsigned char* command;
    // array holding the most recent blob data; plus two bytes for size
    //   sizeof(unsigned short)+ACTS_TOTAL_MAX_SIZE
    unsigned char* data;

    // constructor 
    //    num is the port number for ACTS to listen on.  use zero for
    //    default (it's 5001/35091 right now)
    //
    //    configfile is path to config file.  use NULL for none (that
    //    would be dumb, of course...).
    //
    //    oldacts is a boolean that tells whether or not to use
    //    the old ACTS
    CVisionDevice(int num, char* configfile, bool oldacts);

    ~CVisionDevice();
    void KillACTS();

    virtual CLock* GetLock( void ){ return &lock; };
    
    int Setup();
    int Shutdown();

    size_t GetData(unsigned char *, size_t maxsize);
    void PutData(unsigned char *, size_t maxsize);

    void GetCommand(unsigned char *, size_t maxsize);
    void PutCommand(unsigned char *, size_t maxsize);

    size_t GetConfig(unsigned char *, size_t maxsize);
    void PutConfig(unsigned char *, size_t maxsize);
};

#endif

