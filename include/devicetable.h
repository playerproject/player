/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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

// since we're using a single character to identify devices, we
// can have 256 (should be enough, i think)
#define MAX_NUM_DEVICES 256

typedef struct
{
  char access;  // r, w, or a (read, write, or all)
  CDevice* devicep; // pointer to the device object
} device_info_t;

class CDeviceTable
{
  private:
    // we'll keep the device info here.
    // yeah, yeah dynamic memory yadadyadayada...
    device_info_t devices[MAX_NUM_DEVICES];
    pthread_mutex_t mutex;

  public:
    CDeviceTable();
    ~CDeviceTable();

    // code is the id for the device (e.g, 's' for sonar)
    // access is the access for the device (e.g., 'r' for sonar)
    // devicep is the controlling object (e.g., sonarDevice for sonar)
    //  
    // returns 0 on success, non-zero on failure (device not added)
    int AddDevice(char code, char access, CDevice* devicep);

    // returns the controlling object for the given code (or NULL
    // on failure)
    CDevice* GetDevice(char code);

    // returns the code for access ('r', 'w', or 'a') for the given 
    // device, or 'e' on failure
    char GetDeviceAccess(char code);

};

#endif
