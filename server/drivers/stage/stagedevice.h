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
#include <driver.h>
#include <stage1p3.h>

// this is the root of the stage device filesystem name
// actual directories have the username and instance appended
// e.g. /tmp/stageIO.vaughan.0
#define IOFILENAME "/tmp/stageIO"

class StageDevice : public Driver
{
  // constructor - info points to a single buffer containing the data,
  // command and configuration buffers.  lock fd is an open
  // filedescriptor. we'll lock the lockbyte'th byte in this file to
  // control access to this device
  public: StageDevice( player_stage_info_t* info, 
                        int lockfd, int lockbyte );

  // used to keep a list of objects
  public: StageDevice* next; 

  // Initialise the device
  //
  public: virtual int Setup();

  // Terminate the device
  //
  public: virtual int Shutdown();
    
  // Read data from the device
  //
  public: virtual size_t GetData(player_device_id_t id,
                                 void* dest, size_t len,
                                 struct timeval* timestamp);

  // Write a command to the device
  //
  public: virtual void PutCommand(player_device_id_t id,
                                  void* src, size_t len,
                                  struct timeval* timestamp);

  // Give the device a chance to update
  public: virtual void Update(void);
    
  // Simulator lock bookkeeping data and init method
  //
  private: int lock_fd;
  private: int lock_byte;
  private: void InstallLock( int fd, int index )
           {lock_fd = fd; lock_byte = index;}

  // these two methods are overrides of the Driver definitions.
  private: virtual void Lock();
  private: virtual void Unlock();

  // Pointer to my Device, which has pointers to data and command
  // buffers
  private: Device* device;


  // Pointer to shared info buffers
  //
  public: player_stage_info_t *m_info;
  public: size_t m_info_len;
};

#endif
