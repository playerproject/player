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
 * code for devicedatatable class.  used to keep track of open devices
 * in C++ client
 */

#include <devicedatatable.h>
#include <messages.h>
#include <playerclient.h>

// initialize the table
DeviceDataTable::DeviceDataTable()
{
  numdevices = 0;
  head = NULL;
}

// tear down the table
DeviceDataTable::~DeviceDataTable()
{
  DeviceDataEntry* thisentry = head;
  DeviceDataEntry* tmpentry;
  // for each registered device, delete it.
  while(thisentry)
  {
    tmpentry = thisentry->next;
    delete thisentry;
    numdevices--;
    thisentry = tmpentry;
  }
}

// internal method for searching list; returns NULL if not found
DeviceDataEntry* DeviceDataTable::GetDeviceEntry(uint16_t device, 
                                                 uint16_t index)
{
  DeviceDataEntry* thisentry;

  for(thisentry = head; thisentry; thisentry=thisentry->next)
  {
    if((thisentry->device == device) && (thisentry->index == index))
      break;
  }

  return(thisentry);
}

//
// add a new device to our table.  if there was already an entry
// for this device, old information is overwritten.  will malloc()
// space for data and command based on sizes given.
//
//  returns 0 on success; -1 on failure (likely due to failed malloc())
int DeviceDataTable::AddDevice(uint16_t device, uint16_t index, uint8_t access, 
                int data_size, int command_size)
{
  DeviceDataEntry* thisentry;
  DeviceDataEntry* preventry;
  // right now, don't check for preexisting device, just overwrite the old
  // device.  shouldn't really come up.
  for(thisentry = head,preventry=NULL; thisentry; 
      preventry=thisentry, thisentry=thisentry->next)
  {
    if((thisentry->device == device) && (thisentry->index == index))
      break;
  }

  if(!thisentry)
  {
    thisentry = new DeviceDataEntry;
    if(preventry)
      preventry->next = thisentry;
    else
      head = thisentry;
    numdevices++;
  }
  
  if(thisentry->data)
    free(thisentry->data);
  if(thisentry->command)
    free(thisentry->command);

  thisentry->device = device;
  thisentry->index = index;
  thisentry->access = access;

  if(!(thisentry->data = malloc(data_size)))
  {
    delete thisentry;
    return(-1);
  }
  if(!(thisentry->command = malloc(command_size)))
  {
    delete thisentry;
    return(-1);
  }

  thisentry->datasize = data_size;
  thisentry->commandsize = command_size;
  return(0);
}

// set the access of an existing device to the indicated access.
//
//  returns 0 on success; -1 on failure (likely because couldn't find
//  the indicated device/index
int DeviceDataTable::SetDeviceAccess(uint16_t device, uint16_t index, 
                                     uint8_t access)
{
  DeviceDataEntry* thisentry;

  if(!(thisentry = GetDeviceEntry(device,index)))
    return(-1);

  thisentry->access = access;

  return(0);
}

// get the access of an existing device
//
//  returns access on success; 'e' on failure (likely because couldn't find
//  the indicated device/index
uint8_t DeviceDataTable::GetDeviceAccess(uint16_t device, uint16_t index)
{
  DeviceDataEntry* thisentry;

  if(!(thisentry = GetDeviceEntry(device, index)))
    return('e');

  return(thisentry->access);
}
    
//
//  a convenience function.  checks to see if the device entry exists;
//  if so, then update it with the indicated access; otherwise,
//  create a new entry, looking up the correct data and comman buffer
//  sizes.
int DeviceDataTable::UpdateAccess(uint16_t device, uint16_t index, 
                                  uint8_t access)
{
  int datasize,commandsize;
  
  if(GetDeviceEntry(device, index))
  {
    return(SetDeviceAccess(device,index,access));
  }

  // didn't find the device. need to create new one
  if(GetDeviceSizes(device, &datasize, &commandsize) == -1)
    return(-1);

  return(AddDevice(device,index, access, datasize, commandsize));
}

// internal method for finding sizes of data and command buffers
// for various buffers.
int DeviceDataTable::GetDeviceSizes(uint16_t device, int* datasize, 
                                    int* commandsize)
{
  switch(device)
  {
    case PLAYER_POSITION_CODE:
      *datasize = POSITION_DATA_BUFFER_SIZE;
      *commandsize = POSITION_COMMAND_BUFFER_SIZE;
      break;
    case PLAYER_SONAR_CODE:
      *datasize = SONAR_DATA_BUFFER_SIZE;
      *commandsize = SONAR_COMMAND_BUFFER_SIZE;
      break;
    case PLAYER_GRIPPER_CODE:
      *datasize = GRIPPER_DATA_BUFFER_SIZE;
      *commandsize = GRIPPER_COMMAND_BUFFER_SIZE;
      break;
    case PLAYER_MISC_CODE:
      *datasize = MISC_DATA_BUFFER_SIZE;
      *commandsize = MISC_COMMAND_BUFFER_SIZE;
      break;
    case PLAYER_LASER_CODE:
      *datasize = LASER_DATA_BUFFER_SIZE;
      *commandsize = LASER_COMMAND_BUFFER_SIZE;
      break;
    case PLAYER_PTZ_CODE:
      *datasize = PTZ_DATA_BUFFER_SIZE;
      *commandsize = PTZ_COMMAND_BUFFER_SIZE;
      break;
    case PLAYER_VISION_CODE:
      *datasize = sizeof(vision_data);
      *commandsize = ACTS_COMMAND_BUFFER_SIZE;
      break;
    default:
      *datasize = 0;
      *commandsize = 0;
      printf("GetDeviceSizes(): Warning: couldn't find buffer sizes for %x\n",
                      device);
      return(-1);
  }
  return(0);
}

