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
 *   class to keep track of available devices.  
 */

#ifndef _DEVICETABLE_H
#define _DEVICETABLE_H

#include <pthread.h>
#include <string.h>

#include <libplayercore/driver.h>
#include <libplayercore/device.h>

class DeviceTable
{
  private:
    // we'll keep the device info here.
    Device* head;
    int numdevices;
    pthread_mutex_t mutex;

  public:
    DeviceTable();
    ~DeviceTable();
        
    // this one is used to fill the instantiated device table
    //
    // id is the id for the device (e.g, 's' for sonar)
    // devicep is the controlling object (e.g., sonarDevice for sonar)
    //  
    int AddDevice(player_devaddr_t addr, Driver* driver);
    
    // returns the controlling object for the given id 
    // (returns NULL on failure)
    Driver* GetDriver(player_devaddr_t addr);

    // returns the string name of the driver in use for the given id 
    // (returns NULL on failure)
    const char* GetDriverName(player_devaddr_t addr);

    // find a device, based on id, and return the pointer (or NULL on
    // failure)
    Device* GetDevice(player_devaddr_t addr);

    // Get the first device entry.
    Device *GetFirstDevice() {return head;}

    // Get the next device entry.
    Device *GetNextDevice(Device *entry) {return entry->next;}

    // Return the number of devices
    int Size() {return(numdevices);}

    // Call ProcessMessages() on each non-threaded driver with non-zero
    // subscriptions
    void UpdateDevices();
};

#endif
