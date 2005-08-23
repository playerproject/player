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

/** @defgroup libplayercore libplayercore
@{
This C++ library defines the device driver API, the message queues used to
move messages between devices, facilities for parsing configuration files
and for loading and instantiating drivers.

The core components of this library are:

- player.h : Defines all message formats.

- Driver : All drivers inherit from this class, and must implement certain
methods.

- Device : An instantiated driver (i.e., a driver bound to an interface) is
a device and can be accessed via a pointer of this type.  Use this class
to, for example, subscribe to a device.

- ConfigFile : Use this class to parse a configuration file

- Message : All messages are of this type

- MessageQueue : Messages are delivered on queues of this type, and every
  driver has one (Driver::InQueue).

- error.h : Error-reporting and debug output facilities.  Don't call
  directly into the stdio library (printf, puts, etc.).  Instead use the
  macros defined in error.h, so that message verbosity can be centrally
  controlled and so that all messsages get logged to .player.
@}
*/


#ifndef _PLAYERCORE_H
#define _PLAYERCORE_H

#include <libplayercore/configfile.h>
#include <libplayercore/device.h>
#include <libplayercore/devicetable.h>
#include <libplayercore/driver.h>
#include <libplayercore/drivertable.h>
#include <libplayercore/error.h>
#include <libplayercore/globals.h>
#include <libplayercore/interface_util.h>
#include <libplayercore/message.h>
#include <libplayercore/player.h>
#include <libplayercore/playercommon.h>
#include <libplayercore/playerconfig.h>
#include <libplayercore/playertime.h>
#include <libplayercore/plugins.h>
#include <libplayercore/wallclocktime.h>
#include <libplayercore/addr_util.h>

#endif
