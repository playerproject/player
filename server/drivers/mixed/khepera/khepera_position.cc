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

/* Copyright (C) 2004
 *   Toby Collett, University of Auckland Robotics Group
 *
 * Header for the khepera robot.  The
 * architecture is similar to the P2OS device, in that the position, IR and
 * power services all need to go through a single serial port and
 * base device class.  So this code was copied from p2osdevice and
 * modified to taste.
 * 
 */

#include <khepera.h>
#include <stdio.h>
#include <string.h>

class KheperaPosition: public Khepera
{
public:
  // constructor just calls Khepera Device constructor
  KheperaPosition(char *interface, ConfigFile *cf, int section) : 
    Khepera( cf, section) {}
  ~KheperaPosition();
  
  virtual size_t GetData(void*,unsigned char *, size_t maxsize,
			 uint32_t *timestamp_sec, 
			 uint32_t *timestamp_usec);

  void PutCommand(void*,unsigned char *, size_t maxsize);
};

Driver * KheperaPosition_Init(char *interface, ConfigFile *cf, int section);
void KheperaPosition_Register(DriverTable *table);

/* Global initialization function.  Checks whether specified interface
 * is supported by the device.
 *
 * returns: pointer to new REBPosition object
 */
Driver *
KheperaPosition_Init(char *interface, ConfigFile *cf, int section)
{
  if (!strcmp( PLAYER_POSITION_STRING)) {
    return ( (Driver *) new KheperaPosition( cf, section));
  } else {
    PLAYER_ERROR1("driver \"khepera_position\" does not support interface \"%s\"\n",
		  interface);
    return NULL;
  }
}

/* registers the driver with the drivertable
 *
 * returns: 
 */
void
KheperaPosition_Register(DriverTable *table) 
{
  table->AddDriver("khepera_position",  KheperaPosition_Init);
}

/* destructor.  just set commanded speed to nil
 *
 * returns:
 */
KheperaPosition::~KheperaPosition()
{
  ((player_khepera_cmd_t *)device_command)->position.xspeed = 0;
  ((player_khepera_cmd_t *)device_command)->position.yawspeed = 0;
  ((player_khepera_cmd_t *)device_command)->position.yaw = 0;
}

/* updates data to go to client
 *
 * returns: size of data copied
 */
size_t
KheperaPosition::GetData(void* client,unsigned char *dest, size_t maxsize,
			    uint32_t *timestamp_sec, uint32_t *timestamp_usec)
{
  Lock();

  *((player_position_data_t*)dest) = 
    ((player_khepera_data_t *)device_data)->position;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  
  Unlock();

  return sizeof(player_position_data_t);
}

/* copies a command into the device's buffer
 *
 * returns:
 */
void
KheperaPosition::PutCommand(void* client,unsigned char *src, size_t size)
{
	Lock();
  if (size != sizeof(player_position_cmd_t) ) {
    fprintf(stderr, "KheperaPosition: PutCommand(): command wrong size. ignoring (%d)/%d\n", size,
	    sizeof(player_position_cmd_t));
  } else {
    ((player_khepera_cmd_t *)device_command)->position = 
      *((player_position_cmd_t *)src);
  }
  	Unlock();
}
