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
 *   methods for accessing and playing the amigobot sounds
 */

#include <stdio.h>
#include <p2os.h>

class P2OSSound: public P2OS 
{
 public:
   ~P2OSSound();
   P2OSSound( ConfigFile* cf, int section) :
           P2OS(interface, cf, section){}
   void PutCommand(void*, unsigned char *, size_t maxsize);
};

Driver* P2OSSound_Init( ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SOUND_STRING))
  {
    PLAYER_ERROR1("driver \"p2os_sound\" does not support interface \"%s\"\n",
                  interface);
    return(NULL);
  }
  else
    return((Driver*)(new P2OSSound(interface, cf, section)));
}

// a driver registration function
void 
P2OSSound_Register(DriverTable* table)
{
  table->AddDriver("p2os_sound", PLAYER_ALL_MODE, P2OSSound_Init);
}

P2OSSound::~P2OSSound()
{
  if(command)
    command->sound.index = 0x00;
}


void P2OSSound::PutCommand(void* client, unsigned char *src, size_t size) 
{
  if(size != sizeof( player_gripper_cmd_t ) )
    puts("P2OSSound::PutCommand(): command wrong size. ignoring.");
  else
    ((player_p2os_cmd_t*)device_command)->sound = 
            *((player_sound_cmd_t*)src);
}

