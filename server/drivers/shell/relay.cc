/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

/*
 * $Id$
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_relay relay
 * @brief General-purpose communications relay driver

The @p relay driver repeats all commands it recieves as data packets to all subscribed clients.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_opaque

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- none

@par Example
@verbatim
driver
(
  name "relay"
  provides ["opaque:0"]
)
@endverbatim

@author Toby Collett

*/
/** @} */

#include <stdlib.h>
#include <string.h>
#if !defined (WIN32)
  #include <sys/time.h>
#endif

// we'll use the C client facilities to connect to the remote server
#include <libplayercore/playercore.h>

class Relay:public Driver
{
  public:
	Relay(ConfigFile* cf, int section)
		: Driver(cf, section, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN,PLAYER_OPAQUE_CODE) {};
  	~Relay() {};

	int Setup() {return 0;};
	int Shutdown() {return 0;};

  private:
    // MessageHandler
    int ProcessMessage (QueuePointer &resp_queue, player_msghdr * hdr, void * data);

};

// initialization function
Driver*
Relay_Init( ConfigFile* cf, int section)
{
  return((Driver*)(new Relay(cf, section)));
}

// a driver registration function
void
relay_Register(DriverTable* table)
{
  table->AddDriver("relay",  Relay_Init);
}

int Relay::ProcessMessage (QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
  Publish(device_addr, PLAYER_MSGTYPE_DATA, hdr->subtype, data, hdr->size);
  return 0;
}
