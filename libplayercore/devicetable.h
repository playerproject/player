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

/*
 * $Id$
 *
 *   class to keep track of available devices.  
 */

#ifndef _DEVICETABLE_H
#define _DEVICETABLE_H

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

#include <pthread.h>
#include <string.h>

#include <libplayercore/driver.h>
#include <libplayercore/device.h>

typedef Driver* (*remote_driver_fn_t) (player_devaddr_t addr, void* arg);

class PLAYERCORE_EXPORT DeviceTable
{
  private:
    // we'll keep the device info here.
    Device* head;
    int numdevices;
    pthread_mutex_t mutex;

    // A factory creation function that the application can set (via
    // AddRemoteDevice).  It will be called when GetDevice fails to find a
    // device in the deviceTable
    remote_driver_fn_t remote_driver_fn;
    // Context arg for remote_driver_fn
    void* remote_driver_arg;

  public:
    DeviceTable();
    ~DeviceTable();
        
    // this one is used to fill the instantiated device table
    //
    // id is the id for the device (e.g, 's' for sonar)
    // devicep is the controlling object (e.g., sonarDevice for sonar)
    //  
    Device* AddDevice(player_devaddr_t addr, Driver* driver, bool havelock=false);
    
    // find a device, based on id, and return the pointer (or NULL on
    // failure)
    Device* GetDevice(player_devaddr_t addr, bool lookup_remote=true);
    
    // find a device, based on id, and return the pointer (or NULL on
    // failure)
    Device* GetDevice(const char* str_addr,
                      bool lookup_remote=true);

    // Get the first device entry.
    Device *GetFirstDevice() {return head;}

    // Get the next device entry.
    Device *GetNextDevice(Device *entry) {return entry->next;}

    // Return the number of devices
    int Size() {return(numdevices);}

    // Call ProcessMessages() on each non-threaded driver with non-zero
    // subscriptions
    void UpdateDevices();

    // Subscribe to each device whose driver is marked 'alwayson'.  Returns
    // 0 on success, -1 on error (at least one driver failed to start).
    //
    // TODO: change the semantics of alwayson to be device-specific, rather
    // than just driver-specific.
    int StartAlwaysonDrivers();

    // Unsubscribe to each device whose driver is marked 'alwayson'.  Returns
    // 0 on success, -1 on error (at least one driver failed to start).
    //
    // Only used for coordinating server shutdown
    //
    // TODO: change the semantics of alwayson to be device-specific, rather
    // than just driver-specific.
    int StopAlwaysonDrivers();

    // Register a factory creation function.  It will be called when
    // GetDevice fails to find a device in the deviceTable.  This function
    // might, for example, locate the device on a remote host (in a
    // transport-dependent manner).
    void AddRemoteDriverFn(remote_driver_fn_t rdf, void* arg);
};

#endif
