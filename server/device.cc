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
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>

#include <playertime.h>
extern PlayerTime* GlobalTime;

// this is the main constructor, used by most non-Stage devices.
// storage will be allocated by this constructor
CDevice::CDevice(size_t datasize, size_t commandsize, 
                 int reqqueuelen, int repqueuelen)
{
  device_data = new unsigned char[datasize];
  device_datasize = device_used_datasize = datasize;
  
  device_command = new unsigned char[commandsize];
  device_commandsize = device_used_commandsize = commandsize;
  
  assert(device_data);
  assert(device_command);

  device_reqqueue = new PlayerQueue(reqqueuelen);
  device_repqueue = new PlayerQueue(repqueuelen);

  assert(device_reqqueue);
  assert(device_repqueue);

  // don't forget to make changes to both constructors!
  // it took me a few hours to figure out that the subscription
  // counter needs to be zeroed in the other constuctor - it produced
  // a very nasty heisenbug. boo.
  subscriptions = 0;

  pthread_mutex_init(&accessMutex,NULL);
  pthread_mutex_init(&setupMutex,NULL);
}
    
// this is the other constructor, used mostly by Stage devices.
// if any of the default Put/Get methods are to be used, then storage for 
// the buffers must allocated, and SetupBuffers() called.
CDevice::CDevice()
{
  // ensure immediate segfault in case any of these are used without
  // SetupBuffers() having been called
  device_datasize = device_commandsize = 0;
  device_used_datasize = device_used_commandsize = 0;
  device_data = device_command = NULL;
  device_reqqueue = device_repqueue = NULL;
 
  // don't forget to make changes to both constructors!
  // it took me a few hours to figure out that the subscription
  // counter needs to be zeroed in the other constuctor - it produced
  // a very nasty heisenbug. boo.
  subscriptions = 0;

  // this may be unnecessary, but what the hell...
  pthread_mutex_init(&accessMutex,NULL);
  pthread_mutex_init(&setupMutex,NULL);
}

// this method is used by devices that allocate their own storage, but wish to
// use the default Put/Get methods
void CDevice::SetupBuffers(unsigned char* data, size_t datasize, 
                           unsigned char* command, size_t commandsize, 
                           unsigned char* reqqueue, int reqqueuelen, 
                           unsigned char* repqueue, int repqueuelen)
{
  device_data = data;
  device_datasize = device_used_datasize = datasize;
  device_command = command;
  device_commandsize = device_used_commandsize = commandsize;
  device_reqqueue = new PlayerQueue(reqqueue, reqqueuelen);
  device_repqueue = new PlayerQueue(repqueue, repqueuelen);
}

int CDevice::GetReply(player_device_id_t* device, void* client, 
                      unsigned short* type, struct timeval* ts, 
                      void* data, size_t len)
{
  int size;

  Lock();
  size = device_repqueue->Match(device, (void*)client, type, ts, data, len);
  Unlock();

  return(size);
}

int CDevice::PutReply(player_device_id_t* device, void* client, 
                      unsigned short type, struct timeval* ts, 
                      void* data, size_t len)
{
  struct timeval curr;
  player_device_id_t id;

  if(ts)
    curr = *ts;
  else
    GlobalTime->GetTime(&curr);

  if(!device)
  {
    // stick a dummy device code on it; when the server calls GetReply,
    // it will know what to do
    id.code = id.index = id.port = 0;
  }
  else
    id = *device;


  Lock();
  device_repqueue->Push(&id, client, type, &curr, data, len);
  Unlock();

  return(0);
}

/* a short form, for common use; assumes zero-length reply and that the
 * originating device can be inferred from the client's subscription 
 * list 
 */
int 
CDevice::PutReply(void* client, unsigned short type)
{
  return(PutReply(NULL, client, type, NULL, NULL, 0));
}

/* another short form; this one allows actual replies */
int 
CDevice::PutReply(void* client, unsigned short type, struct timeval* ts, 
                  void* data, size_t len)
{
  return(PutReply(NULL, client, type, ts, data, len));
}

size_t 
CDevice::GetConfig(player_device_id_t* device, void** client, 
                   void *data, size_t len)
{
  int size;

  Lock();

  if((size = device_reqqueue->Pop(device, client, data, len)) < 0)
  {
    Unlock();
    return(0);
  }

  Unlock();
  return(size);
}

size_t 
CDevice::GetConfig(void** client, void *data, size_t len)
{
  return(GetConfig(NULL, client, data, len));
}

int CDevice::PutConfig(player_device_id_t* device, void* client, 
                       void* data, size_t len)
{
  Lock();

  if(device_reqqueue->Push(device, client, PLAYER_MSGTYPE_REQ, NULL, 
                           data, len) < 0)
  {
    // queue was full
    Unlock();
    return(-1);
  }

  Unlock();
  return(0);
}

size_t CDevice::GetData(unsigned char* dest, size_t len,
                        uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  int size;
  Lock();

  assert(len >= device_used_datasize);
  memcpy(dest,device_data,device_used_datasize);
  size = device_used_datasize;
  if(timestamp_sec)
    *timestamp_sec = data_timestamp_sec;
  if(timestamp_usec)
    *timestamp_usec = data_timestamp_usec;

  Unlock();
  return(size);
}

void CDevice::PutData(unsigned char* src, size_t len,
                      uint32_t timestamp_sec, uint32_t timestamp_usec)
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

  // store the amount we copied, for later reference
  device_used_datasize = len;
  Unlock();
}

size_t CDevice::GetCommand( unsigned char* dest, size_t len)
{
  int size;
  Lock();
  assert(len >= device_used_commandsize);
  memcpy(dest,device_command,device_used_commandsize);
  size = device_used_commandsize;
  Unlock();
  return(size);
}

void CDevice::PutCommand( unsigned char* src, size_t len)
{
  Lock();
  assert(len <= device_commandsize);
  memcpy(device_command,src,len);
  // store the amount we wrote
  device_used_commandsize = len;
  Unlock();
}

void CDevice::Lock()
{
  pthread_mutex_lock(&accessMutex);
}

void CDevice::Unlock()
{
  pthread_mutex_unlock(&accessMutex);
}
    
int CDevice::Subscribe(void *client)
{
  int setupResult;

  if(subscriptions == 0) 
  {
    setupResult = Setup();
    if (setupResult == 0 ) 
      subscriptions++; 
  }
  else 
  {
    subscriptions++;
    setupResult = 0;
  }
  
  return( setupResult );
}

int CDevice::Unsubscribe(void *client) 
{
  int shutdownResult;

  if(subscriptions == 0) 
    shutdownResult = -1;
  else if ( subscriptions == 1) 
  {
    shutdownResult = Shutdown();
    if (shutdownResult == 0 ) 
      subscriptions--;
    /* do we want to unsubscribe even though the shutdown went bad? */
  }
  else 
  {
    subscriptions--;
    shutdownResult = 0;
  }
  
  return( shutdownResult );
}

/* start a thread that will invoke Main() */
void 
CDevice::StartThread()
{
  pthread_create(&devicethread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
CDevice::StopThread()
{
  void* dummy;
  pthread_cancel(devicethread);
  if(pthread_join(devicethread,&dummy))
    perror("CDevice::StopThread:pthread_join()");
}

/* Dummy main (just calls real main) */
void* 
CDevice::DummyMain(void *devicep)
{
  // block signals that should be handled by the server thread
#if HAVE_SIGBLOCK
  sigblock(SIGINT);
  sigblock(SIGHUP);
  sigblock(SIGTERM);
#endif

  // Run the overloaded Main() in the subclassed device.
  ((CDevice*)devicep)->Main();
  
  return NULL;
}

void
CDevice::Main() 
{
  fputs("ERROR: You have called Run(), but didn't provide your own Main()!", 
        stderr);
}

// A helper method for internal use; e.g., when one device wants to make a
// request of another device
int
CDevice::Request(player_device_id_t* device, void* requester, 
                 void* request, size_t reqlen,
                 unsigned short* reptype, struct timeval* ts,
                 void* reply, size_t replen)
{
  int size = -1;

  if(PutConfig(device, requester, request, reqlen))
  {
    // queue was full
    *reptype = PLAYER_MSGTYPE_RESP_ERR;
    size = 0;
  }
  else
  {
    // poll for the reply
    for(size = GetReply(device, requester, reptype, ts, reply, replen);
        size < 0;
        usleep(10000), 
        size = GetReply(device, requester, reptype, ts, reply, replen));
  }

  return(size);
}
