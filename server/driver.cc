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
#include <assert.h>

#include "playertime.h"
#include "driver.h"
#include "error.h"
#include "devicetable.h"
#include "clientmanager.h"

#include "configfile.h"
#include "deviceregistry.h"

extern PlayerTime* GlobalTime;
extern DeviceTable* deviceTable;
//extern ClientManager* clientmanager;

// Default constructor for single-interface drivers.  Specify the
// interface code and buffer sizes.
Driver::Driver(ConfigFile *cf, int section, 
               bool overwrite_cmds, size_t queue_maxlen, 
               int interface, uint8_t access)
{
  this->error = 0;
  
  // Look for our default device id
  if(cf->ReadDeviceId(&this->device_id, section, "provides", 
                      interface, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  
  this->subscriptions = 0;
  this->entries = 0;
  this->alwayson = false;

  // Create an interface 
  if(this->AddInterface(this->device_id, access) != 0)
  {
    this->SetError(-1);    
    return;
  }

  this->InQueue = new MessageQueue(overwrite_cmds, queue_maxlen);
  assert(InQueue);

  pthread_mutex_init(&this->accessMutex,NULL);
  pthread_mutex_init(&this->condMutex,NULL);
  pthread_cond_init(&this->cond,NULL);
}
    
// this is the other constructor, used by multi-interface drivers.
Driver::Driver(ConfigFile *cf, int section,
               bool overwrite_cmds, size_t queue_maxlen)
{
  this->device_id.code = INT_MAX;
  
  this->error = 0;
  this->subscriptions = 0;
  this->entries = 0;
  this->alwayson = false;

  this->InQueue = new MessageQueue(overwrite_cmds, queue_maxlen);
  assert(InQueue);

  pthread_mutex_init(&this->accessMutex,NULL);
  pthread_mutex_init(&this->condMutex,NULL);
  pthread_cond_init(&this->cond,NULL);
}

// destructor, to free up allocated buffers.  not stricly necessary, since
// drivers are only destroyed when Player exits, but it is cleaner.
Driver::~Driver()
{
  delete InQueue;
}

// Add an interface
int 
Driver::AddInterface(player_device_id_t id, unsigned char access)
{
  // Add ourself to the device table
  if(deviceTable->AddDevice(id, access, this) != 0)
  {
    PLAYER_ERROR("failed to add interface");
    return -1;
  }
  return 0;
}

// New-style: Write general msg to device
void
Driver::PutMsg(player_device_id_t id, 
	       ClientData* client, 
      	       uint8_t type, 
	       uint8_t subtype,
   	       void* src, 
	       size_t len,
	       struct timeval* timestamp)
{
  struct timeval ts;

  // Fill in the time structure if not supplied
  if(timestamp)
    ts = *timestamp;
  else
    GlobalTime->GetTime(&ts);

  Lock();
  clientmanager->PutMsg(type, subtype, id.code, id.index, &ts,
			len, (unsigned char *)src, client);
  Unlock();
}

// New-style: Read configuration reply from device
/*int 
Driver::GetMsg(player_device_id_t id, void* client, 
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
  //size = device->msgqueue->Match(&id, client, type, timestamp, dest, len);
  size = 0;
  // **** need to use new message queue ****
  Unlock();

  return size;
}
*/

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
    subscriptions--;
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
  PLAYER_ERROR("You have called StartThread(), "
               "but didn't provide your own Main()!");
}

void
Driver::MainQuit() 
{
}

/// Call this to automatically process messages using registered handler
/// Processes messages until no messages remaining in the queue or
/// a message with no handler is reached
void Driver::ProcessMessages()
{
  uint8_t RespData[PLAYER_MAX_MESSAGE_SIZE];
  // If we have subscriptions, then see if we have and pending messages
  // and process them
  MessageQueueElement * el;
  while((el=InQueue->Pop()))
  {
    int RespLen = PLAYER_MAX_MESSAGE_SIZE;

    player_msghdr * hdr = el->msg.GetHeader();
    uint8_t * data = el->msg.GetPayload();

    if (el->msg.GetPayloadSize() != hdr->size)
      PLAYER_WARN2("Message Size does not match msg header, %d != %d\n",el->msg.GetSize() - sizeof(player_msghdr),hdr->size);

    player_device_id_t id;
    id.code = hdr->device;
    id.index = hdr->device_index;
    int ret = ProcessMessage(el->msg.Client, hdr, data, RespData, &RespLen);
    if(ret > 0)
      PutMsg(id, el->msg.Client, ret, hdr->subtype, RespData, RespLen, NULL);
    else if(ret < 0)
    {
      PLAYER_WARN5("Unhandled message for driver device=%d:%d type=%d subtype=%d len=%d\n",hdr->device, hdr->device_index, hdr->type, hdr->subtype, hdr->size);

      // If it was a request, reply with an empty NACK
      if(hdr->type == PLAYER_MSGTYPE_REQ)
        PutMsg(id, el->msg.Client, PLAYER_MSGTYPE_RESP_NACK, 
               hdr->subtype, NULL, 0, NULL);
    }
    pthread_testcancel();
  }
}

int Driver::ProcessMessage(ClientData * client, uint16_t Type, 
                           player_device_id_t device,
                           int size, uint8_t * data, 
                           uint8_t * resp_data, int * resp_len)
{
  player_msghdr hdr;
  hdr.stx = PLAYER_STXX;
  hdr.type=Type;
  hdr.device=device.code;
  hdr.device_index=device.index;
  hdr.timestamp_sec=0;
  hdr.timestamp_usec=0;
  hdr.size=size; // size of message data	

  return ProcessMessage(client, &hdr, data, resp_data, resp_len);
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
  if(!clientmanager)
    PLAYER_WARN("tried to call DataAvailable() on NULL clientmanager!");
  else
    clientmanager->DataAvailable();
}

// a static version that can be used as a callback - rtv
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

