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
#include <arenalock.h>
#include <stage.h>

class CStageDevice : public CDevice
{
    // Minimal constructor
    // buffer points to a single buffer containing the data, command and configuration buffers.
    //
    public: CStageDevice(void *buffer, size_t data_len, size_t command_len, size_t config_len);

    // Initialise the device
    //
    public: virtual int Setup();

    // Terminate the device
    //
    public: virtual int Shutdown();
    
    // Read data from the device
    //
    public: virtual size_t GetData( unsigned char *, size_t maxsize);

    // Write data to the device
    //
    public: virtual void PutData( unsigned char *, size_t maxsize) {};

    // Read a command from  the device
    //
    public: virtual void GetCommand( unsigned char *, size_t maxsize) {};
    
    // Write a command to the device
    //
    public: virtual void PutCommand( unsigned char * , size_t maxsize);

    // Read configuration from the device
    //
    public: virtual size_t GetConfig( unsigned char *, size_t maxsize) {return 0;};

    // Write configuration to the device
    //
    public: virtual void PutConfig( unsigned char *, size_t maxsize);
    
    // Get a lockable object for synchronizing data exchange
    //
    public: virtual CLock* GetLock( void ) {return &m_lock;};

    // Simulator lock
    //
    private: CArenaLock m_lock;

    // Pointer to shared info buffers
    //
    private: player_stage_info_t *m_info;
    private: size_t m_info_len;

    // Pointer to shared data buffers
    //
    private: void *m_data_buffer;
    private: size_t m_data_len;

    // Pointer to shared command buffers
    //
    private: void *m_command_buffer;
    private: size_t m_command_len;

    // Pointer to shared config buffers
    //
    private: void *m_config_buffer;
    private: size_t m_config_len;
};

#endif
