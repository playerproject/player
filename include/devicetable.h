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

// one element in a linked list
class CDeviceEntry
{
  public:
    int port;            // the player tcp port to which this device is tied
    player_device_id_t id;  // id for this device
    unsigned char access;   // allowed access mode: 'r', 'w', or 'a'
    char name[PLAYER_MAX_DEVICE_STRING_LEN]; // the string name for this device
    CDevice* (*initfunc)(int,char**);
    CDevice* devicep;  // the device itself
    CDeviceEntry* next;  // next in list

    CDeviceEntry() { name[0]='\0'; devicep = NULL; next = NULL; }
    ~CDeviceEntry() { if(devicep) delete devicep; }
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
    
    // this is the 'base' AddDevice method, which sets all the fields
    int AddDevice(player_device_id_t id, unsigned char access, 
                  char* name, CDevice* (*initfunc)(int,char**),
                  CDevice* devicep);

    // this one is used to fill the instantiated device table
    //
    // code is the id for the device (e.g, 's' for sonar)
    // access is the access for the device (e.g., 'r' for sonar)
    // devicep is the controlling object (e.g., sonarDevice for sonar)
    //  
    int AddDevice(player_device_id_t id, unsigned char access, 
                  CDevice* devicep);

    // this one sets some different fields; it's used to fill the available
    // device table, instead of the instantiated device table
    int AddDevice(unsigned short code, char access, char* name,
                  CDevice* (*initfunc)(int,char**));


    // returns the controlling object for the given id 
    // (returns NULL on failure)
    CDevice* GetDevice(player_device_id_t id);

    // another one; this one matches on the string name
    CDeviceEntry* GetDeviceEntry(char* name);

    // returns the code for access ('r', 'w', or 'a') for the given 
    // device, or 'e' on failure
    unsigned char GetDeviceAccess(player_device_id_t id);
};

#endif
