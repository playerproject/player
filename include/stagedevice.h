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

///////////////////////////////////////////////////////////////////////////
//
// File: stagedevice.hh
// Author: Andrew Howard
// Date: 28 Nov 2000
// Desc: Class for stage (simulated) devices
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#ifndef _STAGEDEVICE_H
#define _STAGEDEVICE_H

// For length-specific types, error macros, etc
//
#include <playercommon.h>

#include <device.h>
#include <stage.h>

// this is the root of the stage device filesystem name
// actual directories have the username and instance appended
// e.g. /tmp/stageIO.vaughan.0
#define IOFILENAME "/tmp/stageIO"

class CStageDevice : public CDevice
{
  // constructor - info points to a single buffer containing the data,
  // command and configuration buffers.  lock fd is an open
  // filedescriptor. we'll lock the lockbyte'th byte in this file to
  // control access to this device
    public: CStageDevice( player_stage_info_t* info, 
			  int lockfd, int lockbyte );

    public: CStageDevice* next; // used to keep a list of objects

    // Initialise the device
    //
    public: virtual int Setup();

    // Terminate the device
    //
    public: virtual int Shutdown();
    
    // Read data from the device
    //
    public: virtual size_t GetData(unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec = NULL, 
                        uint32_t* timestamp_usec = NULL);

    // Read data from the device and mark the data buffer as empty
    //
    //public: virtual size_t ConsumeData( unsigned char *, size_t );

    // Write a command to the device
    //
    public: virtual void PutCommand( unsigned char * , size_t maxsize);

    // Write configuration to the device
    //
    public: virtual void PutConfig( unsigned char *, size_t maxsize);
    
    // Simulator lock bookkeeping data and init method
    //
    private: int lock_fd;
    private: int lock_byte;
    private: bool InstallLock( int fd, int index )
              {lock_fd = fd; lock_byte = index;}

    // these two methods are overrides of the CDevice definitions.
    private: virtual void Lock();
    private: virtual void Unlock();

    // Pointer to shared info buffers
    //
    public: player_stage_info_t *m_info;
    public: size_t m_info_len;

    // Pointer to shared data buffers
    //
    protected: void *m_data_buffer;
    protected: size_t m_data_len;

    // Pointer to shared command buffers
    //
    protected: void *m_command_buffer;
    protected: size_t m_command_len;

    // Pointer to shared config buffers
    //
    protected: void *m_config_buffer;
    protected: size_t m_config_len;

    // DEPRECATED
    // Pointer to shared truth buffers
    //
    //public: void *m_truth_buffer;
    //public: size_t m_truth_len;
};

#endif
