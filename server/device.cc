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
 * A device entry describes an instantiated driver/interface
 * combination.  Drivers may support more than one interface,
 * and hence appear more than once in the devicetable.
 */

#include <string.h>
#include "driver.h"
#include "device.h"


// Default constructor
Device::Device(player_device_id_t id, Driver *device, unsigned char access)
{
  this->index = -1;
  this->next = NULL;

  this->id = id;
  this->driver = device;
  this->access = access;

  memset(this->drivername, 0, sizeof(this->drivername));
  memset(this->robotname, 0, sizeof(this->robotname));

  if (this->driver)
  {
    this->driver->entries++;
    this->driver->device_id = id;
  }
    
  memset(&this->data_timestamp,0,sizeof(struct timeval));
  memset(&this->command_timestamp,0,sizeof(struct timeval));
  this->data_size = 0;
  this->data_used_size = 0;
  this->data = NULL;
  this->command_size = 0;
  this->command_used_size = 0;
  this->command = NULL;
  this->reqqueue = NULL;
  this->repqueue = NULL;

  this->allocp = false;

  return;
}


Device::~Device() 
{ 
  // Shutdown and delete the associated driver
  if (this->driver) 
  {
    if (this->driver->subscriptions > 0)
      this->driver->Shutdown();
    
    // Delete only if this is the last entry for this driver
    this->driver->entries--;
    if (this->driver->entries == 0)
      delete this->driver;
  }

  // if we allocated our bufferes, delete them
  if(this->allocp)
  {
    if (this->data)
      delete [] this->data;
    if (this->command)
      delete [] this->command;
  }

  // always delete the queues; they're smart enough to know whether their
  // memory was pre-allocated.
  if (this->reqqueue)
    delete this->reqqueue;
  if (this->repqueue)
    delete this->repqueue;

  return;
}


// Initialize the buffers for this interface
void Device::SetupBuffers(size_t datasize, size_t commandsize,
                          size_t reqqueuelen, size_t repqueuelen)
{
  this->data_size = datasize;
  this->data = new unsigned char[datasize];
  this->command_size = commandsize;
  this->command = new unsigned char[commandsize];

  this->reqqueue = new PlayerQueue(reqqueuelen);
  this->repqueue = new PlayerQueue(repqueuelen);

  assert(this->data);
  assert(this->command);
  assert(this->reqqueue);
  assert(this->repqueue);

  this->allocp = true;

  return;
}

void Device::SetupBuffers(void* data, size_t datasize, 
                          void* command, size_t commandsize, 
                          void* reqqueue, int reqqueuelen, 
                          void* repqueue, int repqueuelen)
{
  this->data = (unsigned char*)data;
  this->data_size = datasize;
  this->command = (unsigned char*)command;
  this->command_size = commandsize;
  this->reqqueue = new PlayerQueue(reqqueue, reqqueuelen);
  this->repqueue = new PlayerQueue(repqueue, repqueuelen);
}

