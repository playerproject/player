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
 *  the base class from which all device classes inherit.  here
 *  we implement some generic methods that most devices will not need
 *  to override
 */

#include <device.h>
#include <string.h>

/* these are necessary to make the static fields visible to the linker */
extern unsigned char* CDevice::device_data;
extern unsigned char* CDevice::device_command;
extern size_t CDevice::device_datasize;
extern size_t CDevice::device_commandsize;
extern PlayerQueue* CDevice::device_reqqueue;
extern PlayerQueue* CDevice::device_repqueue;
    
// this is the main constructor, used by most non-Stage devices.
// storage will be allocated by this constructor
CDevice::CDevice(size_t datasize, size_t commandsize, 
                 int reqqueuelen, int repqueuelen)
{
  //printf("CDevice allocating constructor:%d:%d:%d:%d\n",
         //datasize, commandsize, reqqueuelen, repqueuelen);
  if(!device_data)
  {
    device_data = new unsigned char[datasize];
    device_datasize = datasize;
  }
  if(!device_command)
  {
    device_command = new unsigned char[commandsize];
    device_commandsize = commandsize;
  }
  
  assert(device_data);
  assert(device_command);

  if(!device_reqqueue)
    device_reqqueue = new PlayerQueue(reqqueuelen);
  if(!device_repqueue)
    device_repqueue = new PlayerQueue(repqueuelen);

  assert(device_reqqueue);
  assert(device_repqueue);
}
    
// this is the other constructor, used mostly by Stage devices.
// storage must be pre-allocated
CDevice::CDevice(unsigned char* data, size_t datasize, 
        unsigned char* command, size_t commandsize, 
        unsigned char* reqqueue, int reqqueuelen, 
        unsigned char* repqueue, int repqueuelen)
{
  //printf("CDevice non-allocating constructor:%d:%d:%d:%d\n",
         //datasize, commandsize, reqqueuelen, repqueuelen);

  device_data = data;
  device_datasize = datasize;
  device_command = command;
  device_commandsize = commandsize;
  device_reqqueue = new PlayerQueue(reqqueue, reqqueuelen);
  device_repqueue = new PlayerQueue(repqueue, repqueuelen);
}

size_t CDevice::GetReply(unsigned char *, size_t)
{
  puts("WARNING: GetReply() not implemented");
  return(0);
}

void CDevice::PutReply(unsigned char * , size_t)
{
  puts("WARNING: GetReply() not implemented");
}

size_t CDevice::GetConfig( unsigned char *, size_t)
{
  return(0);
}

void CDevice::PutConfig( unsigned char * , size_t)
{
}

size_t CDevice::GetData( unsigned char* dest, size_t len)
{
  assert(len >= device_datasize);
  memcpy(dest,device_data,device_datasize);
  return(device_datasize);
}


void CDevice::PutData(unsigned char* src, size_t len)
{
  assert(len <= device_datasize);
  memcpy(device_data,src,len);
}

void CDevice::GetCommand( unsigned char* dest, size_t len)
{
  assert(len >= device_commandsize);
  memcpy(dest,device_command,device_commandsize);
  //return(device_commandsize);
}

void CDevice::PutCommand( unsigned char* src, size_t len)
{
  assert(len <= device_commandsize);
  memcpy(device_command,src,len);
}

