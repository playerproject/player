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
  ((player_p2os_cmd_t*)device_command)->gripper.cmd = GRIPstore;
  ((player_p2os_cmd_t*)device_command)->gripper.arg = 0x00;
}

size_t CGripperDevice::GetData(unsigned char *dest, size_t maxsize,
                 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Lock();
  *((player_gripper_data_t*)dest) = 
          ((player_p2os_data_t*)device_data)->gripper;
  *timestamp_sec = data_timestamp_sec;
  *timestamp_usec = data_timestamp_usec;
  Unlock();

  return(sizeof( player_gripper_data_t));
}

void CGripperDevice::PutCommand(unsigned char *src, size_t size) 
{
  if(size != sizeof( player_gripper_cmd_t ) )
    puts("CGripperDevice::PutCommand(): command wrong size. ignoring.");
  else
    ((player_p2os_cmd_t*)device_command)->gripper = 
            *((player_gripper_cmd_t*)src);
}

