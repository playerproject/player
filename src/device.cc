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
#include <errno.h>
#include <netinet/in.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

/* these are necessary to make the static fields visible to the linker */
extern unsigned char* CDevice::device_data;
extern unsigned char* CDevice::device_command;
extern size_t CDevice::device_datasize;
extern size_t CDevice::device_commandsize;
extern PlayerQueue* CDevice::device_reqqueue;
extern PlayerQueue* CDevice::device_repqueue;
//extern pthread_mutex_t CDevice::accessMutex;
//extern int CDevice::subscriptions;
    
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

  pthread_mutex_init(&accessMutex,NULL);
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

  // this isn't really needed, but what the hell...
  pthread_mutex_init(&accessMutex,NULL);
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

size_t CDevice::GetConfig(unsigned char * data,size_t len)
{
  return(0);
}

void CDevice::PutConfig(unsigned char * data, size_t len)
{
}

size_t CDevice::GetData(unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec = NULL, 
                        uint32_t* timestamp_usec = NULL)
{
  int size;
  puts("CDevice::GetData");
  Lock();

  assert(len >= device_datasize);
  memcpy(dest,device_data,device_datasize);
  size = device_datasize;
  if(timestamp_sec)
    *timestamp_sec = htonl(data_timestamp_sec);
  if(timestamp_usec)
    *timestamp_usec = htonl(data_timestamp_usec);

  Unlock();
  return(size);
}

void CDevice::PutData(unsigned char* src, size_t len,
                      uint32_t timestamp_sec = 0, 
                      uint32_t timestamp_usec = 0)
{
  if (timestamp_sec == 0)
  {
    struct timeval curr;
    if(GlobalTime->GetTime(&curr) == -1)
      fputs("CDevice::PutData(): GetTime() failed!!!!\n", stderr);
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }
  Lock();
  assert(len <= device_datasize);
  memcpy(device_data,src,len);
  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;
  Unlock();
}

size_t CDevice::GetCommand( unsigned char* dest, size_t len)
{
  int size;
  Lock();
  assert(len >= device_commandsize);
  memcpy(dest,device_command,device_commandsize);
  size = device_commandsize;
  Unlock();
  return(size);
}

void CDevice::PutCommand( unsigned char* src, size_t len)
{
  Lock();
  assert(len <= device_commandsize);
  memcpy(device_command,src,len);
  Unlock();
}

void CDevice::Lock()
{
  pthread_mutex_lock(&accessMutex);
  //puts("done");
}

void CDevice::Unlock()
{
  //puts("CDevice::UnLock()");
  pthread_mutex_unlock(&accessMutex);
  //puts("done");
}

int CDevice::Subscribe()
{
  int setupResult;

  Lock();

  if(subscriptions == 0) 
  {
    setupResult = Setup();
    if (setupResult == 0 ) 
    {
      subscriptions++; 
      subscrcount++; // this one is non-static, and so will be per-device
    }
  }
  else 
  {
    subscriptions++;
    subscrcount++; // this one is non-static, and so will be per-device
    setupResult = 0;
  }
  
  Unlock();
  return( setupResult );
}

int CDevice::Unsubscribe() 
{
  int shutdownResult;

  Lock();
  
  if(subscriptions == 0) 
  {
    shutdownResult = -1;
  }
  else if ( subscriptions == 1) 
  {
    shutdownResult = Shutdown();
    if (shutdownResult == 0 ) 
    { 
      subscriptions--;
      subscrcount--;
    }
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else {
    subscriptions--;
    subscrcount--;
    shutdownResult = 0;
  }
  
  Unlock();

  return( shutdownResult );
}

