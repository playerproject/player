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
 * Driver for the gyro that comes with some Pioneer 3s.
 */

#include <p2os.h>

class P2OSGyro: public P2OS 
{
 public:

   P2OSGyro(char* interface, ConfigFile* cf, int section);
   size_t GetData(void*, unsigned char *, size_t maxsize, 
                   uint32_t* timestamp_sec, uint32_t* timestamp_usec);
};

P2OSGyro::P2OSGyro(char* interface, ConfigFile* cf, int section) : 
  P2OS(interface, cf, section)
{
  P2OS::gyro = 1;    // Activate the gyro code in p2os.cc
}

CDevice* P2OSGyro_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"p2os_gyro\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new P2OSGyro(interface, cf, section)));
}

// a driver registration function
void 
P2OSGyro_Register(DriverTable* table)
{
  table->AddDriver("p2os_gyro", PLAYER_READ_MODE, P2OSGyro_Init);
}

size_t P2OSGyro::GetData(void* client,unsigned char *dest, size_t maxsize,
                            uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_position_data_t*)dest) = 
          ((player_p2os_data_t*)device_data)->gyro;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof( player_position_data_t ));
}

