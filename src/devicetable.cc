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

#include <string.h> // for strncpy(3)

// true if we're connecting to Stage instead of a real robot
extern bool use_stage;

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
    
// this is the 'base' AddDevice method, which sets all the fields
int 
CDeviceTable::AddDevice(int port, unsigned short code, unsigned short index, 
                        unsigned char access, char* name, 
                        CDevice* (*initfunc)(int,char**),
                        CDevice* devicep)
{
  CDeviceEntry* thisentry;
  CDeviceEntry* preventry;
  // right now, don't check for preexisting device, just overwrite the old
  // device.  shouldn't really come up.
  pthread_mutex_lock(&mutex);
  for(thisentry = head,preventry=NULL; thisentry; 
      preventry=thisentry, thisentry=thisentry->next)
  {
    if((thisentry->port == port) && 
       (thisentry->code == code) && 
       (thisentry->index == index))
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

  thisentry->port = port;
  thisentry->code = code;
  thisentry->index = index;
  thisentry->access = access;
  if(name)
  {
    strncpy(thisentry->name,name,sizeof(thisentry->name));
    thisentry->name[sizeof(thisentry->name)-1] = '\0';
  }
  thisentry->initfunc = initfunc;
  thisentry->devicep = devicep;
  pthread_mutex_unlock(&mutex);
  return(0);
}

// returns the controlling object for the given code (or NULL
// on failure)
CDevice* 
CDeviceTable::GetDevice(int port, unsigned short code, 
                        unsigned short index)
{
  CDeviceEntry* thisentry;
  CDevice* devicep = NULL;
  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    // if we're not in Stage, then we're only listening on one port,
    // so we don't need to match the port.  actually, this is a hack to
    // get around the fact that, given arbitrary ordering of command-line
    // arguments, devices can get added to the devicetable with an incorrect
    // port.
    if((thisentry->code == code) && 
       (thisentry->index == index) &&
       (!use_stage || (thisentry->port == port)))
    {
      devicep = thisentry->devicep;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
  return(devicep);
}
    
// another one; this one matches on the string name
CDeviceEntry* 
CDeviceTable::GetDeviceEntry(char* name)
{
  CDeviceEntry* thisentry;
  CDeviceEntry* retval = NULL;
  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if(!strcmp(thisentry->name,name))
    {
      retval = thisentry;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
  return(retval);
}

// returns the code for access ('r', 'w', or 'a') for the given 
// device, or 'e' on failure
unsigned char CDeviceTable::GetDeviceAccess(int port, unsigned short code, 
                                            unsigned short index)
{
  CDeviceEntry* thisentry;
  char access = 'e';

  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if((thisentry->port == port) && 
       (thisentry->code == code) && 
       (thisentry->index == index))
    {
      access = thisentry->access;
      break;
    }
  }
  pthread_mutex_unlock(&mutex);
  return(access);
}

