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

#include <netinet/in.h> /* for htons() */
#include <stdio.h> /* for htons() */
#include <string.h> /* for memcpy() */
#include <positiondevice.h>

CPositionDevice::~CPositionDevice()
{
  //*(short*)&command[POSITION_COMMAND_OFFSET] = 
          //(short)htons((unsigned short)0);
  //*(short*)&command[POSITION_COMMAND_OFFSET+sizeof(short)] = 
          //(short)htons((unsigned short)0);

  command->position.speed = 0;
  command->position.turnrate = 0;
}

size_t CPositionDevice::GetData( unsigned char *dest, size_t maxsize ) 
{
  //memcpy( dest, &data[POSITION_DATA_OFFSET], POSITION_DATA_BUFFER_SIZE);
  *((player_position_data_t*)dest) = data->position;
  return(POSITION_DATA_BUFFER_SIZE);
}

void CPositionDevice::PutCommand( unsigned char *src, size_t size ) 
{
  if(size != POSITION_COMMAND_BUFFER_SIZE)
    puts("CPositionDevice::PutCommand(): command wrong size. ignoring.");
  else
    command->position = *((player_position_cmd_t*)src);
    
  //memcpy( command+POSITION_COMMAND_OFFSET, src, POSITION_COMMAND_BUFFER_SIZE);
}

