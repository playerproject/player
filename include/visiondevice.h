/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 * $Id$
 *
 *   the P2 vision device.  it takes pan tilt zoom commands for the
 *   sony PTZ camera (if equipped), and returns color blob data gathered
 *   from ACTS, which this device spawns and then talks to.
 */

#ifndef _VISIONDEVICE_H
#define _VISIONDEVICE_H

#include <pthread.h>

#include <lock.h>
#include <device.h>
#include <messages.h>

// note: acts_version_t is declared in defaults.h

#define ACTS_VERSION_1_0_STRING "1.0"
#define ACTS_VERSION_1_2_STRING "1.2"

class CVisionDevice:public CDevice 
{
  private:
    pthread_t thread;   // the thread that continuously reads from ACTS
    int debuglevel;             // debuglevel 0=none, 1=basic, 2=everything
    int pid;      // ACTS's pid so we can kill it later

    CLock lock;
    
    // returns the enum representation of the given version string, or
    // ACTS_VERSION_UNKNOWN on failure to match.
    acts_version_t version_string_to_enum(char* versionstr);
    
    // writes the string representation of the given version number into
    // versionstr, up to len.
    // returns  0 on success
    //         -1 on failure to match.
    int version_enum_to_string(acts_version_t versionnum, char* versionstr, 
                               int len);


  public:
    int sock;               // laser device file descriptor
    int portnum;  // port number where we'll run ACTS
    char configfilepath[MAX_FILENAME_SIZE];  // path to configfile
    char binarypath[MAX_FILENAME_SIZE];  // path to executable
    acts_version_t acts_version;  // the above string converted to an enum
    int header_len; // length of incoming packet header (varies by version)
    int header_elt_len; // length of each header element (varies by version)
    int blob_size;  // size of each incoming blob (varies by version)

    // where the most recent data sits, ACTS-encoded
    //player_internal_vision_data_t* data;
    

    // constructor 
    //
    //    takes argc,argv from command-line args
    //
    CVisionDevice(int argc, char** argv);

    ~CVisionDevice();
    void KillACTS();

    virtual CLock* GetLock( void ){ return &lock; };
    
    int Setup();
    int Shutdown();

    size_t GetData(unsigned char *, size_t maxsize);
    //void PutData(unsigned char *, size_t maxsize);

    void GetCommand(unsigned char *, size_t maxsize);
    void PutCommand(unsigned char *, size_t maxsize);

    size_t GetConfig(unsigned char *, size_t maxsize);
    void PutConfig(unsigned char *, size_t maxsize);
};

#endif

