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

#include <device.h>
#include <pthread.h>
#include <configfile.h>
#include <string.h>

// one element in a linked list
class CDeviceEntry
{
  public:
    player_device_id_t id;  // id for this device
    unsigned char access;   // allowed access mode: 'r', 'w', or 'a'
    // the string name for the driver
    char drivername[PLAYER_MAX_DEVICE_STRING_LEN]; 
    // the string name for the robot (only used with Stage)
    char robotname[PLAYER_MAX_DEVICE_STRING_LEN]; 
    CDevice* devicep;  // the device itself
    CDeviceEntry* next;  // next in list

    CDeviceEntry();
    ~CDeviceEntry() 
    { 
      if(devicep) 
      {
        if(devicep->subscriptions)
          devicep->Shutdown();
        delete devicep;
      }
    }
};

class CDeviceTable
{
  private:
    // we'll keep the device info here.
    CDeviceEntry* head;
    int numdevices;
    pthread_mutex_t mutex;

  public:
    CDeviceTable();
    ~CDeviceTable();
    
    // find a device entry, based on id, and return the pointer (or NULL
    // on failure)
    CDeviceEntry* GetDeviceEntry(player_device_id_t id);
    
    // this one is used to fill the instantiated device table
    //
    // id is the id for the device (e.g, 's' for sonar)
    // name is the string driver name (e.g., "p2os_sonar")
    // access is the access for the device (e.g., 'r' for sonar)
    // devicep is the controlling object (e.g., sonarDevice for sonar)
    //  
    int AddDevice(player_device_id_t id, char* drivername, 
                  char* robotname, unsigned char access, 
                  CDevice* devicep);

    // returns the controlling object for the given id 
    // (returns NULL on failure)
    CDevice* GetDevice(player_device_id_t id);
    
    // returns the string name of the driver in use for the given id 
    // (returns NULL on failure)
    char* GetDriver(player_device_id_t id);

    // returns the code for access ('r', 'w', or 'a') for the given 
    // device, or 'e' on failure
    unsigned char GetDeviceAccess(player_device_id_t id);

    // Get the first device entry.
    CDeviceEntry *GetFirstEntry() {return head;}

    // Get the next device entry.
    CDeviceEntry *GetNextEntry(CDeviceEntry *entry) {return entry->next;}

    int Size() {return(numdevices);}
};


// Pointer to the global device table.
extern CDeviceTable* deviceTable;

#endif
