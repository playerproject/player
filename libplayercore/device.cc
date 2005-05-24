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

#include <libplayercore/driver.h>
#include <libplayercore/device.h>

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
}

