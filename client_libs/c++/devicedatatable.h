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
 *   class to keep track of open devices, their commands and data
 */

#ifndef _DEVICEDATATABLE_H
#define _DEVICEDATATABLE_H

#include <stdlib.h>
#include <playercommon.h> // for types

// one element in a linked list
class DeviceDataEntry
{
  public:
    unsigned short device; // the name by which we identify this kind of device
    unsigned short index;  // which device we mean
    unsigned char access;   // 'r', 'w', or 'a' (others?)
    unsigned long long timestamp;  // time at which this data was sensed
    unsigned long long senttime;  // time at which this data was sent
    unsigned long long rectime;  // time at which this data was receive
    void* data;           // buffer for incoming data
    void* command;        // buffer for outgoing commands
    int datasize;         // size of databuffer
    int commandsize;      // size of command buffer
    DeviceDataEntry* next;  // next in list

    DeviceDataEntry() { data = NULL; command = NULL; next = NULL; timestamp=0; }
    ~DeviceDataEntry() { if(data) free(data); if(command) free(command); }
};

class DeviceDataTable
{
  private:
    int numdevices;


  public:
    // linked list of entries
    DeviceDataEntry* head;

    DeviceDataTable();
    ~DeviceDataTable();
    
    // (mostly) internal method for searching list; returns NULL if not found
    DeviceDataEntry* GetDeviceEntry(uint16_t device, uint16_t index);

    //
    // add a new device to our table.  if there was already an entry
    // for this device, old information is overwritten.  will malloc()
    // space for data and command based on sizes given.
    //
    //  returns 0 on success; -1 on failure (likely due to failed malloc())
    int AddDevice(uint16_t device, uint16_t index, uint8_t access, 
                    int data_size, int command_size);
    
    // method for finding sizes of data and command buffers
    // for various buffers.
    int GetDeviceSizes(uint16_t device, int* datasize, int* commandsize);

    //
    //  a convenience function.  checks to see if the device entry exists;
    //  if so, then update it with the indicated access; otherwise,
    //  create a new entry, looking up the correct data and comman buffer
    //  sizes.
    int UpdateAccess(uint16_t device, uint16_t index, uint8_t access);

    // set the access of an existing device to the indicated access.
    //
    //  returns 0 on success; -1 on failure (likely because couldn't find
    //  the indicated device/index
    int SetDeviceAccess(uint16_t device, uint16_t index, uint8_t access);

    // get the access of an existing device
    //
    //  returns access on success; 'e' on failure (likely because couldn't find
    //  the indicated device/index
    uint8_t GetDeviceAccess(uint16_t device, uint16_t index);
};

#endif

