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
#include "clientmanager.h"

#include "configfile.h"
#include "deviceregistry.h"

extern PlayerTime* GlobalTime;
extern DeviceTable* deviceTable;
extern ClientManager* clientmanager;

// Default constructor for single-interface drivers.  Specify the
// interface code and buffer sizes.
Driver::Driver(ConfigFile *cf, int section, int interface, uint8_t access,
               size_t datasize, size_t commandsize, 
               size_t reqqueuelen, size_t repqueuelen)
{
  player_device_id_t* ids;
  int num_ids;

  // Parse devices section
  if((num_ids = cf->ParseDeviceIds(section,&ids)) < 0)
  {
    this->SetError(-1);    
    return;
  }

  // Take on the first id given in the config file, but make sure it has
  // the right code
  this->device_id = ids[0];
  if((interface > 0) && (this->device_id.code != interface))
  {
    PLAYER_ERROR2("section [%d]: interface %s is not supported",
                  section, ::lookup_interface_name(0,ids[0].code));
    this->SetError(-1);
    return;
  }

  // consume the id, then check for superfluous ids
  ids[0].port = 0;
  if(cf->UnusedIds(section, ids,num_ids))
  {
    this->SetError(-1);    
    free(ids);
    return;
  }
  free(ids);

  // Create an interface 
  if (this->AddInterface(this->device_id, access,
                         datasize, commandsize, reqqueuelen, repqueuelen) != 0)
  {
    this->SetError(-1);    
    return;
  }

  subscriptions = 0;
  entries = 0;
  alwayson = false;
  error = 0;

  pthread_mutex_init(&accessMutex,NULL);
  pthread_mutex_init(&condMutex,NULL);
  pthread_cond_init(&cond,NULL);
}
    
// this is the other constructor, used mostly by Stage devices.
// if any of the default Put/Get methods are to be used, then storage for 
// the buffers must allocated, and SetupBuffers() called.
Driver::Driver(ConfigFile *cf, int section)
{
  device_id.code = INT_MAX;
  
  subscriptions = 0;
  entries = 0;
  alwayson = false;
  error = 0;

  pthread_mutex_init(&accessMutex,NULL);
  pthread_mutex_init(&condMutex,NULL);
  pthread_cond_init(&cond,NULL);
}

// destructor, to free up allocated buffers.  not stricly necessary, since
// drivers are only destroyed when Player exits, but it is cleaner.
Driver::~Driver()
{
}

// Add an interface
int 
Driver::AddInterface(player_device_id_t id, unsigned char access,
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

// Add an interface, with pre-allocated memory
int 
Driver::AddInterface(player_device_id_t id, unsigned char access,
                     void* data, size_t datasize, 
                     void* command, size_t commandsize, 
                     void* reqqueue, int reqqueuelen, 
                     void* repqueue, int repqueuelen)
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
  device->SetupBuffers(data, datasize, command, commandsize, 
                       reqqueue, reqqueuelen, repqueue, repqueuelen);

  return 0;
}


// New-style PutData; [id] specifies the interface to be written
void 
Driver::PutData(player_device_id_t id,
                void* src, size_t len,
                struct timeval* timestamp)
{
  Device *device;
  struct timeval ts;
  
  // If the timestamp is not set, fill it out with the current time
  if(timestamp)
    ts = *timestamp;
  else
    GlobalTime->GetTime(&ts);

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    // Ignore, on the assumption that this id refers to an interface
    // supported by the driver but not requested by the user.  This allows
    // the driver to always PutData() to all its supported interfaces,
    // without checking whether the user wants to use them.
    return;
  }

  // Store data in the device buffer
  Lock();
  assert(len <= device->data_size);
  memcpy(device->data, src, len);
  device->data_timestamp = ts;
  device->data_used_size = len;
  Unlock();
  
  // signal that new data is available
  DataAvailable();
}


// New-style GetData; [id] specifies the interface to be read
size_t 
Driver::GetData(player_device_id_t id,
                void* dest, size_t len,
                struct timeval* timestamp)
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
  if(timestamp)
    *timestamp = device->data_timestamp;
  Unlock();
  
  return size;
}


// New-style: Write a new command to the device
void 
Driver::PutCommand(player_device_id_t id,
                   void* src, size_t len,
                   struct timeval* timestamp)
{
  Device *device;
  struct timeval ts;
  
  // If the timestamp is not set, fill it out with the current time
  if(timestamp)
    ts = *timestamp;
  else
    GlobalTime->GetTime(&ts);

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
  device->command_timestamp = ts;
  device->command_used_size = len;
  Unlock();

  return;
}


// New-style: Read the current command for the device
size_t 
Driver::GetCommand(player_device_id_t id,
                   void* dest, size_t len,
                   struct timeval* timestamp)
{
  int size;
  Device *device;
  
  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    // Ignore, on the assumption that this id refers to an interface
    // supported by the driver but not requested by the user.  This allows
    // the driver to always GetCommand() from all its supported interfaces,
    // without checking whether the user wants to use them.
    return(0);
  }
  
  Lock();
  assert(len >= device->command_used_size);
  memcpy(dest,device->command,device->command_used_size);
  size = device->command_used_size;
  if(timestamp)
    *timestamp = device->command_timestamp;
  Unlock();
  
  return(size);
}


// New-style: Clear the current command buffer
void
Driver::ClearCommand(player_device_id_t id)
{
  Device *device;
  
  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  Lock();
  device->command_used_size = 0;
  Unlock();

  return;
}


// New-style: Write configuration request to device
int 
Driver::PutConfig(player_device_id_t id, void* client, 
                  void* src, size_t len,
                  struct timeval* timestamp)
{
  Device *device;
  int retval;
  struct timeval ts;

  if(timestamp)
    ts = *timestamp;
  else
    GlobalTime->GetTime(&ts);

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    PLAYER_ERROR("interface not found; did you AddInterface()?");
    assert(false);
  }

  // Push onto request queue
  Lock();
  retval = device->reqqueue->Push(&id, client, PLAYER_MSGTYPE_REQ, 
                                  &ts, src, len);
  Unlock();

  if(retval < 0)
    return(-1);
  else
    return(0);
}


// New-style: Get next configuration request for device
int 
Driver::GetConfig(player_device_id_t id, void **client, 
                  void* dest, size_t len,
                  struct timeval* timestamp)
{
  int size;
  Device *device;

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    // Ignore, on the assumption that this id refers to an interface
    // supported by the driver but not requested by the user.  This allows
    // the driver to always GetConfig() from all its supported interfaces,
    // without checking whether the user wants to use them.
    return(0);
  }

  // Pop device from request queue
  Lock();
  if((size = device->reqqueue->Pop(&id, client, timestamp, dest, len)) < 0)
  {
    Unlock();
    return(0);
  }
  Unlock();
  
  return(size);
}

// New-style: Write configuration reply to device
int 
Driver::PutReply(player_device_id_t id, void* client, 
                 unsigned short type, 
                 void* src, size_t len,
                 struct timeval* timestamp)
{
  Device *device;
  struct timeval ts;
  int retval;

  // Fill in the time structure if not supplies
  if(timestamp)
    ts = *timestamp;
  else
    GlobalTime->GetTime(&ts);

  // Find the matching device in the device table
  device = deviceTable->GetDevice(id);
  if (device == NULL)
  {
    // Ignore, on the assumption that this id refers to an interface
    // supported by the driver but not requested by the user.  This allows
    // the driver to always PutReply() to all its supported interfaces,
    // without checking whether the user wants to use them.
    return(0);
  }

  Lock();
  retval = device->repqueue->Push(&id, client, type, &ts, src, len);
  Unlock();

  if(retval < 0)
    return retval;
  else
    return 0;
}

// New-style: Read configuration reply from device
int 
Driver::GetReply(player_device_id_t id, void* client, 
                 unsigned short* type, 
                 void* dest, size_t len,
                 struct timeval* timestamp)
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
  size = device->repqueue->Match(&id, client, type, timestamp, dest, len);
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
    
int Driver::Subscribe(player_device_id_t id)
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

int Driver::Unsubscribe(player_device_id_t id)
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
Driver::StartThread(void)
{
  pthread_create(&driverthread, NULL, &DummyMain, this);
}

/* cancel (and wait for termination) of the thread */
void 
Driver::StopThread(void)
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
  fputs("ERROR: You have called StartThread(), but didn't provide your own Main()!", 
        stderr);
}

void
Driver::MainQuit() 
{
}

// A helper method for internal use; e.g., when one device wants to make a
// request of another device
int
Driver::Request(player_device_id_t id, void* requester, 
                void* request, size_t reqlen,
                struct timeval* req_timestamp,
                unsigned short* reptype, 
                void* reply, size_t replen,
                struct timeval* rep_timestamp)
{
  int size = -1;

  if(PutConfig(id, requester, request, reqlen, req_timestamp) < 0)
  {
    // queue was full
    *reptype = PLAYER_MSGTYPE_RESP_ERR;
    size = 0;
  }
  else
  {
    // poll for the reply
    for(size = GetReply(id, requester, reptype, reply, replen, rep_timestamp);
        size < 0;
        usleep(10000),
        size = GetReply(id, requester, reptype, reply, replen, rep_timestamp));
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
  clientmanager->DataAvailable();
}

// a static version that can be used as a callback from libraries that
// don't know about Player (e.g. Stage). - rtv
void 
Driver::DataAvailableStatic( Driver* driver )
{
  driver->DataAvailable();
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

// do we still need this?
#if 0
size_t Driver::GetNumData(void* client)
{
  return(1);
}
#endif

