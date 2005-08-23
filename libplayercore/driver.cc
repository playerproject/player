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

#include <libplayercore/playertime.h>
#include <libplayercore/driver.h>
#include <libplayercore/error.h>
#include <libplayercore/devicetable.h>
#include <libplayercore/configfile.h>
#include <libplayercore/globals.h>

// Default constructor for single-interface drivers.  Specify the
// interface code and buffer sizes.
Driver::Driver(ConfigFile *cf, int section, 
               bool overwrite_cmds, size_t queue_maxlen, 
               int interf)
{
  this->error = 0;
  driverthread = 0;
  
  // Look for our default device id
  if(cf->ReadDeviceAddr(&this->device_addr, section, "provides", 
                        interf, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  
  this->subscriptions = 0;
  this->entries = 0;
  this->alwayson = false;

  // Create an interface 
  if(this->AddInterface(this->device_addr) != 0)
  {
    this->SetError(-1);    
    return;
  }

  this->InQueue = new MessageQueue(overwrite_cmds, queue_maxlen);
  assert(InQueue);
  pthread_mutex_init(&this->accessMutex,NULL);
}
    
// this is the other constructor, used by multi-interface drivers.
Driver::Driver(ConfigFile *cf, int section,
               bool overwrite_cmds, size_t queue_maxlen)
{
  this->error = 0;
  this->driverthread = 0;
  
  this->device_addr.interf = INT_MAX;
  
  this->subscriptions = 0;
  this->alwayson = false;
  this->entries = 0;

  this->InQueue = new MessageQueue(overwrite_cmds, queue_maxlen);
  assert(InQueue);

  pthread_mutex_init(&this->accessMutex,NULL);
}

// destructor, to free up allocated queue.
Driver::~Driver()
{
  delete this->InQueue;
}

// Add an interface
int 
Driver::AddInterface(player_devaddr_t addr)
{
  // Add ourself to the device table
  if(deviceTable->AddDevice(addr, this) != 0)
  {
    PLAYER_ERROR("failed to add interface");
    return -1;
  }
  return 0;
}

void
Driver::Publish(MessageQueue* queue, 
                player_msghdr_t* hdr,
                void* src)
{
  Device* dev;

  Message msg(*hdr,src,hdr->size);

  if(queue)
  {
    // push onto the given queue, which provides its own locking
    queue->Push(msg);
  }
  else
  {
    // lock here, because we're accessing our device's queue list
    this->Lock();
    // push onto each queue subscribed to the given device
    if(!(dev = deviceTable->GetDevice(hdr->addr)))
    {
      PLAYER_ERROR("tried to publish message via non-existent device");
      this->Unlock();
      return;
    }
    for(size_t i=0;i<dev->len_queues;i++)
    {
      if(dev->queues[i])
        dev->queues[i]->Push(msg);
    }
    this->Unlock();
  }
}

void
Driver::Publish(player_devaddr_t addr, 
                MessageQueue* queue, 
                uint8_t type, 
                uint8_t subtype,
                void* src, 
                size_t len,
                double* timestamp)
{
  struct timeval ts;
  double t;

  // Fill in the time structure if not supplied
  if(timestamp)
  {
    t = *timestamp;
  }
  else
  {
    GlobalTime->GetTime(&ts);
    t = ts.tv_sec + ts.tv_usec/1e6;
  }

  player_msghdr_t hdr;
  memset(&hdr,0,sizeof(player_msghdr_t));
  //hdr.stx = PLAYER_STXX;
  hdr.addr = addr;
  hdr.type = type;
  hdr.subtype = subtype;
  hdr.timestamp = t;
  hdr.size = len;

  this->Publish(queue, &hdr, src);
}

void Driver::Lock()
{
  pthread_mutex_lock(&accessMutex);
}

void Driver::Unlock()
{
  pthread_mutex_unlock(&accessMutex);
}
    
int Driver::Subscribe(player_devaddr_t addr)
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
  
  return(setupResult);
}

int Driver::Unsubscribe(player_devaddr_t addr)
{
  int shutdownResult;

  if(subscriptions == 0) 
    shutdownResult = -1;
  else if ( subscriptions == 1) 
  {
    shutdownResult = Shutdown();
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
  // Install a cleanup function
  pthread_cleanup_push(&DummyMainQuit, devicep);

  // Run the overloaded Main() in the subclassed device.
  ((Driver*)devicep)->Main();

  // Run the uninstall cleanup function
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
  void* RespData = NULL;
  size_t RespLen;
 
  // If we have subscriptions, then see if we have any pending messages
  // and process them
  Message* msg;
  while((msg = this->InQueue->Pop()))
  {
    player_msghdr * hdr = msg->GetHeader();
    void * data = msg->GetPayload();

    if (msg->GetPayloadSize() != hdr->size)
      PLAYER_WARN2("Message Size does not match msg header, %d != %d\n",msg->GetSize() - sizeof(player_msghdr),hdr->size);

    int ret = this->ProcessMessage(msg->Queue, hdr, data, &RespData, &RespLen);
    if(ret > 0)
    {
      this->Publish(hdr->addr, msg->Queue, ret, 
                    hdr->subtype, RespData, RespLen, NULL);
      if(RespLen)
        free(RespData);
    }
    else if(ret < 0)
    {
      PLAYER_WARN5("Unhandled message for driver "
                   "device=%d:%d type=%d subtype=%d len=%d\n",
                   hdr->addr.interf, hdr->addr.index, 
                   hdr->type, hdr->subtype, hdr->size);

      // If it was a request, reply with an empty NACK
      if(hdr->type == PLAYER_MSGTYPE_REQ)
        this->Publish(hdr->addr, msg->Queue, PLAYER_MSGTYPE_RESP_NACK, 
                      hdr->subtype, NULL, 0, NULL);
    }
    delete msg;
    pthread_testcancel();
  }
}

