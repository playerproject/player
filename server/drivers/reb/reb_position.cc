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

/* Copyright (C) 2002
 *   John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 * 
 * the REB position device implementation.  basically copied from
 * positiondevice.cc
 *
 */

#include <reb.h>
#include <stdio.h>
#include <string.h>

class REBPosition: public REB
{
public:
  // constructor just calls REBDevice constructor
  REBPosition(char *interface, ConfigFile *cf, int section) : 
    REB(interface, cf, section) {}
  ~REBPosition();
  
  virtual size_t GetData(unsigned char *, size_t maxsize,
			 uint32_t *timestamp_sec, 
			 uint32_t *timestamp_usec);

  void PutCommand(unsigned char *, size_t maxsize);
};

CDevice * REBPosition_Init(char *interface, ConfigFile *cf, int section);
void REBPosition_Register(DriverTable *table);

/* Global initialization function.  Checks whether specified interface
 * is supported by the device.
 *
 * returns: pointer to new REBPosition object
 */
CDevice *
REBPosition_Init(char *interface, ConfigFile *cf, int section)
{
  if (!strcmp(interface, PLAYER_POSITION_STRING)) {
    return ( (CDevice *) new REBPosition(interface, cf, section));
  } else {
    PLAYER_ERROR1("driver \"reb_position\" does not support interface \"%s\"\n",
		  interface);
    return NULL;
  }
}

/* registers the driver with the drivertable
 *
 * returns: 
 */
void
REBPosition_Register(DriverTable *table) 
{
  table->AddDriver("reb_position", PLAYER_ALL_MODE, REBPosition_Init);
}

/* destructor.  just set commanded speed to nil
 *
 * returns:
 */
REBPosition::~REBPosition()
{
  ((player_reb_cmd_t *)device_command)->position.xspeed = 0;
  ((player_reb_cmd_t *)device_command)->position.yawspeed = 0;
  ((player_reb_cmd_t *)device_command)->position.yaw = 0;
}

/* updates data to go to client
 *
 * returns: size of data copied
 */
size_t
REBPosition::GetData(unsigned char *dest, size_t maxsize,
			    uint32_t *timestamp_sec, uint32_t *timestamp_usec)
{
  Lock();

  *((player_position_data_t*)dest) = 
    ((player_reb_data_t *)device_data)->position;
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
REBPosition::PutCommand(unsigned char *src, size_t size)
{
  if (size != sizeof(player_position_cmd_t) ) {
    fprintf(stderr, "REBPosition: PutCommand(): command wrong size. ignoring (%d)/%d\n", size,
	    sizeof(player_position_cmd_t));
  } else {
    ((player_reb_cmd_t *)device_command)->position = 
      *((player_position_cmd_t *)src);
  }
}
