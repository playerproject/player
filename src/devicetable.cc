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
  numdevices = 0;
  head = NULL;
  pthread_mutex_init(&mutex,NULL);
}

// tear down the table
CDeviceTable::~CDeviceTable()
{
  CDeviceEntry* thisentry = head;
  CDeviceEntry* tmpentry;
  // for each registered device, delete it.
  pthread_mutex_lock(&mutex);
  while(thisentry)
  {
    tmpentry = thisentry->next;
    delete thisentry;
    numdevices--;
    thisentry = tmpentry;
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
int CDeviceTable::AddDevice(unsigned short code, unsigned short index, 
                            unsigned char access, CDevice* devicep)
{
  CDeviceEntry* thisentry;
  CDeviceEntry* preventry;
  // right now, don't check for preexisting device, just overwrite the old
  // device.  shouldn't really come up.
  pthread_mutex_lock(&mutex);
  for(thisentry = head,preventry=NULL; thisentry; 
      preventry=thisentry, thisentry=thisentry->next)
  {
    if((thisentry->code == code) && (thisentry->index == index))
    {
      if(thisentry->devicep)
        delete thisentry->devicep;
      break;
    }
  }

  if(!thisentry)
  {
    thisentry = new CDeviceEntry;
    if(preventry)
      preventry->next = thisentry;
    else
      head = thisentry;
    numdevices++;
  }

  thisentry->code = code;
  thisentry->index = index;
  thisentry->access = access;
  thisentry->devicep = devicep;
  pthread_mutex_unlock(&mutex);
  return(0);
}

// returns the controlling object for the given code (or NULL
// on failure)
CDevice* CDeviceTable::GetDevice(unsigned short code, unsigned index)
{
  CDeviceEntry* thisentry;
  CDevice* devicep = NULL;
  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if((thisentry->code == code) && (thisentry->index == index))
    {
      devicep = thisentry->devicep;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
  return(devicep);
}

// returns the code for access ('r', 'w', or 'a') for the given 
// device, or 'e' on failure
unsigned char CDeviceTable::GetDeviceAccess(unsigned short code, 
                                            unsigned short index)
{
  CDeviceEntry* thisentry;
  char access = 'e';

  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if((thisentry->code == code) && (thisentry->index == index))
    {
      access = thisentry->access;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
  return(access);
}

