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
 * A device is a driver/interface pair.
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#include <libplayercore/player.h>

// Forward declarations
class Driver;

// A device entry describes an instantiated driver/interface
// combination.  Drivers may support more than one interface,
// and hence appear more than once in the device table.
class Device
{
  public:
  
  // Default constructor etc
  Device(player_device_id_t id, Driver *driver, unsigned char access);
  ~Device();

  // Index in the device table
  int index;              

  // Next entry in the device table (this is a linked-list)
  Device* next;

  // Id for this device
  player_device_id_t id;

  // Allowed access mode: 'r', 'w', or 'a'
  unsigned char access;   

  // The string name for the driver
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];
  
  // the string name for the robot (only used with Stage)
  char robotname[PLAYER_MAX_DEVICE_STRING_LEN];

  // Pointer to the driver
  Driver* driver;
};

#endif
