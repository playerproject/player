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
#include <string.h> // for strncpy(3)
#include "devicetable.h"
#include "deviceregistry.h"

// true if we're connecting to Stage instead of a real robot
extern bool use_stage;


// initialize the table
DeviceTable::DeviceTable()
{
  numdevices = 0;
  head = NULL;
  pthread_mutex_init(&mutex,NULL);
}

// tear down the table
DeviceTable::~DeviceTable()
{
  Device* thisentry = head;
  Device* tmpentry;
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
DeviceTable::AddDevice(player_device_id_t id, unsigned char access, Driver* driver)
{
  Device* thisentry;
  Device* preventry;

  pthread_mutex_lock(&mutex);

  // Check for duplicate entries (not allowed)
  for(thisentry = head,preventry=NULL; thisentry; 
      preventry=thisentry, thisentry=thisentry->next)
  {
    if ((thisentry->id.port == id.port) && 
        (thisentry->id.code == id.code) && 
        (thisentry->id.index == id.index))
      break;
  }
  if (thisentry)
  {
    PLAYER_ERROR3("duplicate device id %d:%s:%d",
                  id.port, lookup_interface_name(0, id.code), id.index);
    pthread_mutex_unlock(&mutex);
    return -1;
  }
    
  // Create a new device entry
  thisentry = new Device(id, driver, access);
  thisentry->index = numdevices;
  thisentry->next = NULL;
  if(preventry)
    preventry->next = thisentry;
  else
    head = thisentry;
  numdevices++;

  pthread_mutex_unlock(&mutex);
  return(0);
}

// returns the controlling object for the given code (or NULL
// on failure)
Driver* 
DeviceTable::GetDriver(player_device_id_t id)
{ 
  Device* thisentry;
  Driver* driver = NULL;

  if((thisentry = GetDevice(id)))
    driver = thisentry->driver;

  return(driver);
}

    
// returns the string name of the driver in use for the given id 
// (returns NULL on failure)
char* 
DeviceTable::GetDriverName(player_device_id_t id)
{
  Device* thisentry;
  char* driver = NULL;

  //printf( "Looking up ID %u.%u.%u\n", id.port, id.code, id.index );

  if((thisentry = GetDevice(id)))
    driver = thisentry->drivername;

  return(driver);
}


// find a device entry, based on id, and return the pointer (or NULL
// on failure)
Device* 
DeviceTable::GetDevice(player_device_id_t id)
{
  Device* thisentry;
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

       // Player now uses multiple ports if multiple robots are
       // specified in the config file, with or without Stage, so we
       // do need to match port too - rtv.  
       //(!use_stage || (thisentry->id.port == id.port)))

       (thisentry->id.port == id.port))
      break;
  }

  pthread_mutex_unlock(&mutex);
  return(thisentry);
}


// returns the code for access ('r', 'w', or 'a') for the given 
// device, or 'e' on failure
unsigned char 
DeviceTable::GetDeviceAccess(player_device_id_t id)
{
  Device* thisentry;
  char access = 'e';

  if((thisentry = GetDevice(id)))
    access = thisentry->access;

  return(access);
}
