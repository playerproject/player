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
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdlib.h>
#include <device.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>

#include "playertime.h"
#include "driver.h"
#include "devicetable.h"

#include "configfile.h"
#include "deviceregistry.h"

extern PlayerTime* GlobalTime;
extern DeviceTable* deviceTable;


// Default constructor for single-interface drivers.  Specify the
// interface code and buffer sizes.
Driver::Driver(ConfigFile *cf, int section, int interface, uint8_t access,
               size_t datasize, size_t commandsize, 
               int reqqueuelen, int repqueuelen)
{
  // Figure out our device id (from the configuration file)
  if (cf->ReadDeviceId(section, 0, interface, &this->device_id) != 0)
  {
    this->SetError(-1);
    return;
  }

  // TODO: driver name and modes
  // Create an interface 
  if (this->AddInterface(this->device_id, access,
                         datasize, commandsize, reqqueuelen, repqueuelen) != 0)
  {
    this->SetError(-1);    
    return;
  }

  // don't forget to make changes to both constructors!
  // it took me a few hours to figure out that the subscription
  // counter needs to be zeroed in the other constuctor - it produced
  // a very nasty heisenbug. boo.
  subscriptions = 0;
  entries = 0;
  alwayson = false;
  error = 0;

  pthread_mutex_init(&accessMutex,NULL);
  pthread_mutex_init(&setupMutex,NULL);
  pthread_mutex_init(&condMutex,NULL);
  pthread_cond_init(&cond,NULL);
}
    
// this is the other constructor, used mostly by Stage devices.
// if any of the default Put/Get methods are to be used, then storage for 
// the buffers must allocated, and SetupBuffers() called.
Driver::Driver(ConfigFile *cf, int section)
{
  // don't forget to make changes to both constructors!
  // it took me a few hours to figure out that the subscription
  // counter needs to be zeroed in the other constuctor - it produced
  // a very nasty heisenbug. boo.
  subscriptions = 0;
  entries = 0;
  alwayson = false;
  error = 0;

  // this may be unnecessary, but what the hell...
  pthread_mutex_init(&accessMutex,NULL);
  pthread_mutex_init(&setupMutex,NULL);
  pthread_mutex_init(&condMutex,NULL);
  pthread_cond_init(&cond,NULL);
}

// destructor, to free up allocated buffers.  not stricly necessary, since
// devices are only destroyed when Player exits, but it is cleaner.
Driver::~Driver()
{
}


// Add an interface
int Driver::AddInterface(player_device_id_t id, unsigned char access,
                           size_t datasize, size_t commandsize,
                           size_t reqqueuelen, size_t repqueuelen)
{
  Device *device;

  // Add ourself to the device table
  if (deviceTable->AddDevice(id, access, this) != 0)
  {
    PLAYER_ERROR("failed to add interface");
    return -1;
  }

  // Get the device and initialize it
  device = deviceTable->GetDevice(id);
  assert(device != NULL);
  device->SetupBuffers(datasize, commandsize, reqqueuelen, repqueuelen);

  return 0;
}


// New-style PutData; [id] specifies the interface to be written
void Driver::PutData(player_device_id_t id,
                        void* src, size_t len,
                        uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  Device *device;
  
  // If the timestamp is not set, fill it out with the current time
  if (timestamp_sec == 0)
  {
    struct timeval curr;
    if(GlobalTime->GetTime(&curr) == -1)
      PLAYER_ERROR("GetTime() failed");
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("data buffer not found; did you AddInterface()?");
    assert(false);
  }

  // Store data in the device buffer
  Lock();
  assert(len <= device->data_size);
  memcpy(device->data, src, len);
  device->data_time_sec = timestamp_sec;
  device->data_time_usec = timestamp_usec;
  device->data_used_size = len;
  Unlock();
  
  // signal that new data is available
  DataAvailable();
}


// New-style GetNumData; [id] specifies the interface to be read
size_t Driver::GetNumData(player_device_id_t id, void* client)
{
  return (1);
}


// New-style GetData; [id] specifies the interface to be read
size_t Driver::GetData(player_device_id_t id, void* client, 
                          unsigned char* dest, size_t len,
                          uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  Device *device;
  size_t size;

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  // Copy the data from the interface
  Lock();
  assert(len >= device->data_used_size);
  memcpy(dest, device->data, device->data_used_size);
  size = device->data_used_size;
  if(timestamp_sec)
    *timestamp_sec = device->data_time_sec;
  if(timestamp_usec)
    *timestamp_usec = device->data_time_usec;
  Unlock();
  
  return size;
}


// New-style: Write a new command to the device
void Driver::PutCommand(player_device_id_t id, void* client, unsigned char* src, size_t len)
{
  Device *device;

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  // Write data to command buffer
  Lock();
  assert(len <= device->command_size);
  memcpy(device->command,src,len);
  device->command_used_size = len;
  Unlock();

  return;
}


// New-style: Read the current command for the device
size_t Driver::GetCommand(player_device_id_t id, void* dest, size_t len)
{
  int size;
  Device *device;
  
  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }
  
  Lock();
  assert(len >= device->command_used_size);
  memcpy(dest,device->command,device->command_used_size);
  size = device->command_used_size;
  Unlock();
  
  return(size);
}



// New-style: Write configuration request to device
int Driver::PutConfig(player_device_id_t id, player_device_id_t *src_id,
                        void *client, void* data, size_t len)
{
  Device *device;

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  // Push onto request queue
  Lock();
  if (device->reqqueue->Push(&id, client, PLAYER_MSGTYPE_REQ, NULL, data, len) < 0)
  {
    Unlock();
    return(-1);
  }
  Unlock();
  
  return(0);
}


// New-style: Get next configuration request for device
size_t Driver::GetConfig(player_device_id_t id, player_device_id_t *src_id,
                           void **client, void *data, size_t len)
{
  int size;
  Device *device;

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  // Pop device from request queue
  Lock();
  if((size = device->reqqueue->Pop(&id, client, data, len)) < 0)
  {
    Unlock();
    return(0);
  }
  Unlock();
  
  return(size);
}



void Driver::PutData(void* src, size_t len,
                      uint32_t timestamp_sec, uint32_t timestamp_usec)
{
  /*
  if (timestamp_sec == 0)
  {
    struct timeval curr;
    if(GlobalTime->GetTime(&curr) == -1)
      PLAYER_ERROR("GetTime() failed");
    timestamp_sec = curr.tv_sec;
    timestamp_usec = curr.tv_usec;
  }
  Lock();
  assert(len <= driver_datasize);
  memcpy(driver_data,src,len);
  data_timestamp_sec = timestamp_sec;
  data_timestamp_usec = timestamp_usec;

  // store the amount we copied, for later reference
  driver_used_datasize = len;
  Unlock();
  
  // signal that new data is available
  DataAvailable();
  */

  this->PutData(this->device_id, src, len, timestamp_sec, timestamp_usec);
  return;
}


size_t Driver::GetNumData(void* client)
{
  return(1);
}


size_t Driver::GetData(void* client, unsigned char* dest, size_t len,
                       uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  /*
  int size;
  Lock();

  assert(len >= driver_used_datasize);
  memcpy(dest,driver_data,driver_used_datasize);
  size = driver_used_datasize;
  if(timestamp_sec)
    *timestamp_sec = data_timestamp_sec;
  if(timestamp_usec)
    *timestamp_usec = data_timestamp_usec;

  Unlock();
  return(size);
  */

  return this->GetData(this->device_id, client, dest, len, timestamp_sec, timestamp_usec);
}


// Write a new command to the device
void Driver::PutCommand(void* client, unsigned char* src, size_t len)
{
  /*
  Lock();
  assert(len <= driver_commandsize);
  memcpy(driver_command,src,len);
  // store the amount we wrote
  driver_used_commandsize = len;
  Unlock();
  */

  this->PutCommand(this->device_id, client, src, len);
  return;
}


// Read the current command for the device
size_t Driver::GetCommand( void* dest, size_t len)
{
  /* REMOVE
  int size;
  Lock();
  assert(len >= driver_used_commandsize);
  memcpy(dest,driver_command,driver_used_commandsize);
  size = driver_used_commandsize;
  Unlock();
  return(size);
  */
  return this->GetCommand(this->device_id, dest, len);
}

// Write configuration request to device
int Driver::PutConfig(player_device_id_t* device, void* client, 
                       void* data, size_t len)
{
  /* REMOVE
  Lock();

  if(driver_reqqueue->Push(device, client, PLAYER_MSGTYPE_REQ, NULL, 
                           data, len) < 0)
  {
    // queue was full
    Unlock();
    return(-1);
  }

  Unlock();
  return(0);
  */
  return this->PutConfig(this->device_id, device, client, data, len);
}



// Read configuration request from device
size_t Driver::GetConfig(player_device_id_t* device, void** client, 
                         void *data, size_t len)
{
  /* REMOVE
  int size;

  Lock();

  if((size = driver_reqqueue->Pop(device, client, data, len)) < 0)
  {
    Unlock();
    return(0);
  }

  Unlock();
  return(size);
  */

  return this->GetConfig(this->device_id, device, client, data, len);
}


// Convenient short form
size_t Driver::GetConfig(void** client, void *data, size_t len)
{
  return(GetConfig(NULL, client, data, len));
}


// Write configuration reply to device
int Driver::PutReply(player_device_id_t* device, void* client, 
                      unsigned short type, struct timeval* ts, 
                      void* data, size_t len)
{
  /* REMOVE
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
  driver_repqueue->Push(&id, client, type, &curr, data, len);
  Unlock();

  return(0);
  */

  return this->PutReply(this->device_id, device, client, type, ts, data, len);
}


/* a short form, for common use; assumes zero-length reply and that the
 * originating device can be inferred from the client's subscription 
 * list 
 */
int 
Driver::PutReply(void* client, unsigned short type)
{
  return(PutReply(NULL, client, type, NULL, NULL, 0));
}


/* another short form; this one allows actual replies */
int 
Driver::PutReply(void* client, unsigned short type, struct timeval* ts, 
                  void* data, size_t len)
{
  return(PutReply(NULL, client, type, ts, data, len));
}


// New-style: Write configuration reply to device
int Driver::PutReply(player_device_id_t id, player_device_id_t *src_id, void* client, 
                        unsigned short type, struct timeval* ts, 
                        void* data, size_t len)
{
  Device *device;
  struct timeval curr;

  // Fill in the time structure if not supplies
  if(ts)
    curr = *ts;
  else
    GlobalTime->GetTime(&curr);

  /* REMOVE
  // Use old-style single interface
  if (!new_style)
    return PutConfig(&id, client, data, len);
  */

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  Lock();
  device->repqueue->Push(&id, client, type, &curr, data, len);
  Unlock();

  return 0;
}



// Read configuration reply from device
int Driver::GetReply(player_device_id_t* device, void* client, 
                      unsigned short* type, struct timeval* ts, 
                      void* data, size_t len)
{
  /* REMOVE
  int size;

  Lock();
  size = driver_repqueue->Match(device, (void*)client, type, ts, data, len);
  Unlock();

  return(size);
  */

  return this->GetReply(this->device_id, device, client, type, ts, data, len);
}


// New-style: Read configuration reply from device
int Driver::GetReply(player_device_id_t id, player_device_id_t *src_id, void* client, 
                        unsigned short* type, struct timeval* ts, 
                        void* data, size_t len)
{
  int size;
  Device *device;

  /* REMOVE
  // Use old-style single interface
  if (!new_style)
    return GetReply(src_id, client, type, ts, data, len);
  */

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  Lock();
  size = device->repqueue->Match(&id, client, type, ts, data, len);
  Unlock();

  return size;

}



void Driver::Lock()
{
  pthread_mutex_lock(&accessMutex);
}

void Driver::Unlock()
{
  pthread_mutex_unlock(&accessMutex);
}
    
int Driver::Subscribe(void *client)
{
  /* printf( "device subscribe %d:%d:%d\n",
    driver_id.port, 
    driver_id.code, 
    driver_id.index ); 
  */

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

int Driver::Unsubscribe(void *client) 
{
  int shutdownResult;

  if(subscriptions == 0) 
    shutdownResult = -1;
  else if ( subscriptions == 1) 
  {
    shutdownResult = Shutdown();
    // to release anybody that's still waiting, in order to allow shutdown
    DataAvailable();
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
Driver::StartThread()
{
  pthread_create(&driverthread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
Driver::StopThread()
{
  void* dummy;
  pthread_cancel(driverthread);
  if(pthread_join(driverthread,&dummy))
    perror("Driver::StopThread:pthread_join()");
}

/* Dummy main (just calls real main) */
void* 
Driver::DummyMain(void *devicep)
{
  // block signals that should be handled by the server thread
#if HAVE_SIGBLOCK
  sigblock(SIGINT);
  sigblock(SIGHUP);
  sigblock(SIGTERM);
  sigblock(SIGUSR1);
#endif

  // Install a cleanup function
  pthread_cleanup_push(&DummyMainQuit, devicep);

  // Run the overloaded Main() in the subclassed device.
  ((Driver*)devicep)->Main();

  // Run, the uninstall cleanup function
  pthread_cleanup_pop(1);
  
  return NULL;
}

/* Dummy main cleanup (just calls real main cleanup) */
void
Driver::DummyMainQuit(void *devicep)
{
  // Run the overloaded MainCleanup() in the subclassed device.
  ((Driver*)devicep)->MainQuit();
}


void
Driver::Main() 
{
  fputs("ERROR: You have called Run(), but didn't provide your own Main()!", 
        stderr);
}

void
Driver::MainQuit() 
{
}

// A helper method for internal use; e.g., when one device wants to make a
// request of another device
int
Driver::Request(player_device_id_t* device, void* requester, 
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
    
// Signal that new data is available (calls pthread_cond_broadcast()
// on this device's condition variable, which will release other
// devices that are waiting on this one).  Usually call this method from 
// PutData().
void 
Driver::DataAvailable(void)
{
  pthread_mutex_lock(&condMutex);
  pthread_cond_broadcast(&cond);
  pthread_mutex_unlock(&condMutex);

  // also wake up the server thread
  //if(kill(g_server_pid,SIGUSR1) < 0)
    //perror("kill(2) failed while waking up server thread");
}

// Waits on the condition variable associated with this device.
void 
Driver::Wait(void)
{
  // need to push this cleanup function, cause if a thread is cancelled while
  // in pthread_cond_wait(), it will immediately relock the mutex.  thus we
  // need to unlock ourselves before exiting.
  pthread_cleanup_push((void(*)(void*))pthread_mutex_unlock,(void*)&condMutex);
  pthread_mutex_lock(&condMutex);
  pthread_cond_wait(&cond,&condMutex);
  pthread_mutex_unlock(&condMutex);
  pthread_cleanup_pop(1);
}

