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
#include <devicetable.h>

// initialize the table
CDeviceTable::CDeviceTable()
{
  for(int i = 0; i < MAX_NUM_DEVICES; i++)
  {
    devices[i].access = 'e';
    devices[i].devicep = (CDevice*)NULL;
  }
  pthread_mutex_init(&mutex,NULL);
}

// tear down the table
CDeviceTable::~CDeviceTable()
{
  // for each registered device, delete it.
  pthread_mutex_lock(&mutex);
  for(int i=0;i<MAX_NUM_DEVICES;i++)
  {
    if(devices[i].devicep)
      delete devices[i].devicep;
  }
  pthread_mutex_unlock(&mutex);
  
  // destroy the mutex.
  pthread_mutex_destroy(&mutex);
}

// code is the id for the device (e.g, 's' for sonar)
// access is the access for the device (e.g., 'r' for sonar)
// devicep is the controlling object (e.g., sonarDevice for sonar)
//  
// returns 0 on success, non-zero on failure (device not added)
int CDeviceTable::AddDevice(char code, char access, CDevice* devicep)
{
  // right now, don't check for error, just overwrite the old
  // device.  shouldn't really come up.

  pthread_mutex_lock(&mutex);
  devices[code].access = access;
  devices[code].devicep = devicep;
  pthread_mutex_unlock(&mutex);
  return(0);
}

// returns the controlling object for the given code (or NULL
// on failure)
CDevice* CDeviceTable::GetDevice(char code)
{
  CDevice* devicep;
  pthread_mutex_lock(&mutex);
  devicep = devices[code].devicep;
  pthread_mutex_unlock(&mutex);
  return(devicep);
}

// returns the code for access ('r', 'w', or 'a') for the given 
// device, or 'e' on failure
char CDeviceTable::GetDeviceAccess(char code)
{
  char access;

  pthread_mutex_lock(&mutex);
  access = devices[code].access;
  pthread_mutex_unlock(&mutex);
  return(access);
}

