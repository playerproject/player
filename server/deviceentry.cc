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
#include "deviceentry.h"


// Default constructor
CDeviceEntry::CDeviceEntry(player_device_id_t id, CDevice *device, unsigned char access,
                           const char *drivername, const char *robotname)
{
  this->index = -1;
  this->next = NULL;

  this->id = id;
  this->devicep = device;
  this->access = access;

  strncpy(this->drivername, drivername, sizeof(this->drivername));
  if (robotname)
    strncpy(this->robotname, robotname, sizeof(this->robotname));

  if (this->devicep)
  {
    this->devicep->entries++;
    this->devicep->device_id = id;
  }
    
  this->data_time_sec = 0;
  this->data_time_usec = 0;
  this->data_size = 0;
  this->data = NULL;
  this->command_size = 0;
  this->command = NULL;

  return;
}


CDeviceEntry::~CDeviceEntry() 
{ 
  // Shutdown and delete the associated driver
  if (this->devicep) 
  {
    if (this->devicep->subscriptions > 0)
      this->devicep->Shutdown();
    
    // Delete only if this is the last entry for this driver
    this->devicep->entries--;
    if (this->devicep->entries == 0)
      delete this->devicep;
  }

  // Delete our buffers
  if (this->data)
    delete [] this->data;
  if (this->command)
    delete [] this->command;
  if (this->reqqueue)
    delete this->reqqueue;
  if (this->repqueue)
    delete this->repqueue;

  return;
}


// Initialize the buffers for this interface
void CDeviceEntry::SetupBuffers(size_t datasize, size_t commandsize,
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

  return;
}
