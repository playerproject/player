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

#include <libplayercore/error.h>
#include <libplayercore/devicetable.h>
#include <libplayercore/interface_util.h>

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
DeviceTable::AddDevice(player_devaddr_t addr, 
                       unsigned char access, 
                       Driver* driver)
{
  Device* thisentry;
  Device* preventry;

  pthread_mutex_lock(&mutex);

  // Check for duplicate entries (not allowed)
  for(thisentry = head,preventry=NULL; thisentry; 
      preventry=thisentry, thisentry=thisentry->next)
  {
    if(Device::MatchDeviceAddress(thisentry->addr, addr))
      break;
  }
  if(thisentry)
  {
    PLAYER_ERROR4("duplicate device addr %X:%d:%s:%d",
                  addr.host, addr.robot,
                  lookup_interface_name(0, addr.interface), 
                  addr.index);
    pthread_mutex_unlock(&mutex);
    return(-1);
  }
    
  // Create a new device entry
  thisentry = new Device(addr, driver, access);
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
DeviceTable::GetDriver(player_devaddr_t addr)
{ 
  Device* thisentry;
  Driver* driver = NULL;

  if((thisentry = GetDevice(addr)))
    driver = thisentry->driver;

  return(driver);
}

    
// returns the string name of the driver in use for the given addr 
// (returns NULL on failure)
const char* 
DeviceTable::GetDriverName(player_devaddr_t addr)
{
  Device* thisentry;
  char* driver = NULL;

  if((thisentry = GetDevice(addr)))
    driver = thisentry->drivername;

  return((const char*)driver);
}


// find a device entry, based on addr, and return the pointer (or NULL
// on failure)
Device* 
DeviceTable::GetDevice(player_devaddr_t addr)
{
  Device* thisentry;
  pthread_mutex_lock(&mutex);
  for(thisentry=head;thisentry;thisentry=thisentry->next)
  {
    if(Device::MatchDeviceAddress(thisentry->addr, addr))
      break;
  }

  pthread_mutex_unlock(&mutex);
  return(thisentry);
}


// returns the code for access ('r', 'w', or 'a') for the given 
// device, or 'e' on failure
unsigned char 
DeviceTable::GetDeviceAccess(player_devaddr_t addr)
{
  Device* thisentry;
  char access = PLAYER_ERROR_MODE;

  if((thisentry = GetDevice(addr)))
    access = thisentry->access;

  return(access);
}
