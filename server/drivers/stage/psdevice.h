/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Richard Vaughan, & Andrew Howard
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
 * Here is defined the PSDevice class, which inherits from Player's CDevice
 * class, but adds a couple of extra methods for those Player devices that 
 * should behave differently when Stage is being used.  In particular, methods
 * are provided here to allow such devices to send information back and forth
 * to Stage for visualization purposes.
 */

#ifndef _PSDEVICE_H
#define _PSDEVICE_H

#if HAVE_CONFIG_H
  #include <config.h>
#endif

// For length-specific types, error macros, etc
//
#include <playercommon.h>
#include <device.h>

#if INCLUDE_STAGE
  #include <stage.h>
#endif

class PSDevice : public CDevice
{
  private:
#if INCLUDE_STAGE
    // Pointer to shared info buffers
    //
    player_stage_info_t *m_info;
    size_t m_info_len;
    
    // buffers for stage data and command
    unsigned char* stage_device_data;
    unsigned char* stage_device_command;

    // maximum sizes of data and command buffers
    size_t stage_device_datasize;
    size_t stage_device_commandsize;
    
    // amount at last write into each respective buffer
    size_t stage_device_used_datasize;
    size_t stage_device_used_commandsize;
  
    // Simulator lock bookkeeping data and init method
    //
    int lock_fd;
    int lock_byte;
    void InstallLock(int fd, int index) {lock_fd = fd; lock_byte = index;}

    void StageLock(void);
    void StageUnlock(void);
#endif

  public: 
#if INCLUDE_STAGE
    // to record the time at which the device gathered the data
    // these are public because one device (e.g., P2OS) might need to set the
    // timestamp of another (e.g., sonar)
    uint32_t stage_data_timestamp_sec;
    uint32_t stage_data_timestamp_usec;
#endif
    
    // constructor, which just invokes the CDevice constructor
    PSDevice(size_t datasize, size_t commandsize, 
             int reqqueuelen, int repqueuelen) : 
            CDevice(datasize, commandsize, reqqueuelen, repqueuelen) {}

#if INCLUDE_STAGE
    // call this to setup pointers to the stage buffer; args the same as 
    // for the StageDevice constructor
    void SetupStageBuffers(player_stage_info_t* info, 
                           int lockfd, int lockbyte );

    void PutStageCommand(void* client, unsigned char* src, size_t len);
    size_t GetStageData(void* client, unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec);
#endif
};

#endif
