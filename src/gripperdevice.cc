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
 *   methods for accessing and controlling the Pioneer 2 gripper
 */

#include <gripperdevice.h>
#include <stdio.h>

CGripperDevice::~CGripperDevice()
{
  command->gripper.cmd = GRIPstore;
  command->gripper.arg = 0x00;
}

size_t CGripperDevice::GetData( unsigned char *dest, size_t maxsize ) 
{
  *((player_gripper_data_t*)dest) = data->gripper;
  return( sizeof( player_gripper_data_t) );
}

void CGripperDevice::PutCommand(unsigned char *src, size_t size) 
{
  if(size != sizeof( player_gripper_cmd_t ) )
    puts("CGripperDevice::PutCommand(): command wrong size. ignoring.");
  else
    command->gripper = *((player_gripper_cmd_t*)src);
}

