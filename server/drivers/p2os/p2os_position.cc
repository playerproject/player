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
 *   the P2 position device.  accepts commands for changing
 *   wheel speeds, and returns data on x,y,theta,compass, etc.
 */

#include <stdio.h>
#include <p2os.h>

class P2OSPosition: public P2OS 
{
 public:
   ~P2OSPosition();
   P2OSPosition(char* interface, ConfigFile* cf, int section) :
           P2OS(interface, cf, section){}
   virtual size_t GetData(unsigned char *, size_t maxsize,
                          uint32_t* timestamp_sec, uint32_t* timestamp_usec);
   void PutCommand( unsigned char *, size_t maxsize);
};

CDevice* P2OSPosition_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_POSITION_STRING))
  {
    PLAYER_ERROR1("driver \"p2os_position\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new P2OSPosition(interface, cf, section)));
}

// a driver registration function
void 
P2OSPosition_Register(DriverTable* table)
{
  table->AddDriver("p2os_position", PLAYER_ALL_MODE, P2OSPosition_Init);
}

P2OSPosition::~P2OSPosition()
{
  ((player_p2os_cmd_t*)device_command)->position.speed = 0;
  ((player_p2os_cmd_t*)device_command)->position.turnrate = 0;
}

size_t P2OSPosition::GetData( unsigned char *dest, size_t maxsize,
                                 uint32_t* timestamp_sec, 
                                 uint32_t* timestamp_usec)
{
  Lock();
  *((player_position_data_t*)dest) = 
          ((player_p2os_data_t*)device_data)->position;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();
  return( sizeof( player_position_data_t) );
}

void P2OSPosition::PutCommand( unsigned char *src, size_t size ) 
{
  if(size != sizeof( player_position_cmd_t ) )
  {
    puts("P2OSPosition::PutCommand(): command wrong size. ignoring.");
    printf("expected %d; got %d\n", sizeof(player_position_cmd_t),size);
  }
  else
    ((player_p2os_cmd_t*)device_command)->position = 
            *((player_position_cmd_t*)src);
}

