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

#include <rflex.h>
#include <string.h>

class RFLEXdio: public RFLEX 
{
 public:

   RFLEXdio(char* interface, ConfigFile* cf, int section) : 
           RFLEX(interface, cf, section){}
   size_t GetData(void*, unsigned char *, size_t maxsize, 
                   uint32_t* timestamp_sec, uint32_t* timestamp_usec);
};

CDevice* RFLEXdio_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_DIO_STRING))
  {
    PLAYER_ERROR1("driver \"rflex_dio\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else{
    CDevice* tmp=((CDevice*)(new RFLEXdio(interface, cf, section)));
    return tmp;
  }
}

// a driver registration function
void 
RFLEXdio_Register(DriverTable* table)
{
  table->AddDriver("rflex_dio", PLAYER_READ_MODE, RFLEXdio_Init);
}

size_t RFLEXdio::GetData(void* client,unsigned char *dest, size_t maxsize,
                            uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_dio_data_t*)dest) = ((player_rflex_data_t*)device_data)->dio;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof( player_dio_data_t ));
}

