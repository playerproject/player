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

#include <string.h>
#include <stdio.h>
#include <khepera.h>

class KheperaIR : public Khepera
{
public:
  KheperaIR(char *interface, ConfigFile *cf, int section) :
    Khepera(interface, cf, section) {}


  virtual size_t GetData(void*, unsigned char *dest, size_t maxsize,
			 uint32_t *timestamp_sec, uint32_t *timestamp_usec);
};

/* initialize the driver.  check for supported interfaces
 *
 * returns: pointer to new REBIR object
 */
Driver *
KheperaIR_Init(char *interface, ConfigFile *cf, int section)
{
  if (!strcmp(interface, PLAYER_IR_STRING)) {
    return (Driver *) new KheperaIR(interface, cf, section);
  } else {
    PLAYER_ERROR1("driver \"khepera_ir\" does not support interface \"%s\"\n",
		  interface);
    
    return NULL;
  }
}

/* register the Khepera IR driver in the drivertable
 *
 * returns: 
 */
void
KheperaIR_Register(DriverTable *table) 
{
  table->AddDriver("khepera_ir", PLAYER_READ_MODE, KheperaIR_Init);
}

/* this puts data in a place for the client to get it
 *
 * returns: number of bytes copied
 */
size_t
KheperaIR::GetData(void* client,unsigned char *dest, size_t maxsize,
		      uint32_t *timestamp_sec, uint32_t *timestamp_usec)
{
  Lock();

  *((player_ir_data_t *)dest) = ((player_khepera_data_t *)device_data)->ir;

  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return sizeof(player_ir_data_t);
}
