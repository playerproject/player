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

CDeviceEntry::CDeviceEntry()
{ 
  devicep = NULL; 
  next = NULL;
  memset(drivername, 0, sizeof(drivername));
  memset(robotname, 0, sizeof(robotname));
}


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
CDeviceTable::AddDevice(player_device_id_t id, char* drivername,
                        char* robotname, unsigned char access, 
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
    if((thisentry->id.port == id.port) && 
       (thisentry->id.code == id.code) && 
       (thisentry->id.index == id.index))
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

  thisentry->id = id;
  strncpy(thisentry->drivername, drivername, sizeof(thisentry->drivername));
  if(robotname)
    strncpy(thisentry->robotname, robotname, sizeof(thisentry->robotname));
  thisentry->access = access;
  thisentry->devicep = devicep;
  if(devicep)
    devicep->device_id = id;
  pthread_mutex_unlock(&mutex);

  /*printf( "Added device %d.%d.%d - %s\n", 
	  thisentry->id.port, 
	  thisentry->id.code, 
	  thisentry->id.index, 
	  thisentry->drivername );
  */

  return(0);
}

// returns the controlling object for the given code (or NULL
// on failure)
CDevice* 
CDeviceTable::GetDevice(player_device_id_t id)
{ 
  CDeviceEntry* thisentry;
  CDevice* devicep = NULL;

  if((thisentry = GetDeviceEntry(id)))
    devicep = thisentry->devicep;

  return(devicep);
}

    
// returns the code for access ('r', 'w', or 'a') for the given 
// device, or 'e' on failure
unsigned char 
CDeviceTable::GetDeviceAccess(player_device_id_t id)
{
  CDeviceEntry* thisentry;
  char access = 'e';

  if((thisentry = GetDeviceEntry(id)))
    access = thisentry->access;

  return(access);
}
// returns the string name of the driver in use for the given id 
// (returns NULL on failure)
char* 
CDeviceTable::GetDriver(player_device_id_t id)
{
  CDeviceEntry* thisentry;
  char* driver = NULL;

  //printf( "Looking up ID %u.%u.%u\n", id.port, id.code, id.index );

  if((thisentry = GetDeviceEntry(id)))
    driver = thisentry->drivername;

  return(driver);
}    

// find a device entry, based on id, and return the pointer (or NULL
// on failure)
CDeviceEntry* 
CDeviceTable::GetDeviceEntry(player_device_id_t id)
{
  CDeviceEntry* thisentry;
  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    // if we're not in Stage, then we're only listening on one port,
    // so we don't need to match the port.  actually, this is a hack to
    // get around the fact that, given arbitrary ordering of command-line
    // arguments, devices can get added to the devicetable with an incorrect
    // port.
    if((thisentry->id.code == id.code) && 
       (thisentry->id.index == id.index) &&
       (!use_stage || (thisentry->id.port == id.port)))
      break;
  }

  pthread_mutex_unlock(&mutex);
  return(thisentry);
}

