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
 * REB Power.  Reads back the battery level
 *
 */

#include <string.h>
#include <stdio.h>
#include <reb.h>

class REBPower: public REB
{
public:

  REBPower(char *interface, ConfigFile *cf, int section) : 
    REB(interface, cf, section) {}
  
  size_t GetData(void*,unsigned char *dest, size_t maxsize,
		 uint32_t *ts_sec, uint32_t *ts_usec);
};

/* checks for supported interfaces
 *
 * returns: pointer to new driver object if supported, null otherwise
 */
CDevice *
REBPower_Init(char *interface, ConfigFile *cf, int section) 
{
  if (!strcmp(interface, PLAYER_POWER_STRING)) {
    return (CDevice *) new REBPower(interface, cf, section);
  } else {
    PLAYER_ERROR1("driver \"reb_power\" does not support interface \"%s\"\n",
		  interface);
    
    return NULL;
  }
}

/* registers driver with drivertable
 *
 * returns: 
 */
void
REBPower_Register(DriverTable *table)
{
  table->AddDriver("reb_power", PLAYER_READ_MODE, REBPower_Init);
}

size_t
REBPower::GetData(void* client,unsigned char *dest, size_t maxsize,
			uint32_t *ts_sec, uint32_t *ts_usec)
{
  Lock();

  *((player_power_data_t *)dest) = ((player_reb_data_t *)device_data)->power;
  *ts_sec = data_timestamp_sec;
  *ts_usec = data_timestamp_usec;
  Unlock();

  return sizeof(player_power_data_t);
}
