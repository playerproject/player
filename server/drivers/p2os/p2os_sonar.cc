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
 *   the P2 sonar device.  takes no commands.  return sonar readings.
 */
#include <stdio.h>
#include <netinet/in.h>
#include <p2os.h>

class P2OSSonar: public P2OS
{
 public:
   P2OSSonar(char* interface, ConfigFile* cf, int section) :
           P2OS(interface, cf, section){}
   
   virtual size_t GetData(void*, unsigned char *, size_t maxsize,
                           uint32_t* timestamp_sec, uint32_t* timestamp_usec);
};

CDevice* P2OSSonar_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SONAR_STRING))
  {
    PLAYER_ERROR1("driver \"p2os_sonar\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new P2OSSonar(interface, cf, section)));
}

// a driver registration function
void 
P2OSSonar_Register(DriverTable* table)
{
  table->AddDriver("p2os_sonar", PLAYER_READ_MODE, P2OSSonar_Init);
}

size_t P2OSSonar::GetData(void* client,unsigned char *dest, size_t maxsize,
                             uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_sonar_data_t*)dest) = ((player_p2os_data_t*)device_data)->sonar;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return( sizeof(player_sonar_data_t) );
}

