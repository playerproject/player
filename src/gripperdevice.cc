/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
#include <string.h> /* for memcpy() */

CGripperDevice::~CGripperDevice()
{
  command[GRIPPER_COMMAND_OFFSET] = GRIPstore;
  command[GRIPPER_COMMAND_OFFSET+1] = 0x00;
}
size_t CGripperDevice::GetData( unsigned char *dest, size_t maxsize ) {
  /*
   * in this order:
   *   ints: time X Y
   *   shorts: heading, forwardvel, turnrate, compass
   *   chars: stalls
   *   shorts: 16 sonars
   *
   *   chars: gripstate,gripbeams
   */
  memcpy( dest, &data[GRIPPER_DATA_OFFSET], GRIPPER_DATA_BUFFER_SIZE );
  return(GRIPPER_DATA_BUFFER_SIZE);
}

void CGripperDevice::PutCommand(unsigned char *src, size_t size) 
{
  if(size != GRIPPER_COMMAND_BUFFER_SIZE)
    puts("CGripperDevice::PutCommand(): command wrong size. ignoring.");
  else
    memcpy( command+GRIPPER_COMMAND_OFFSET, src, GRIPPER_COMMAND_BUFFER_SIZE);
}

