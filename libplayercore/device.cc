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
 * A device entry describes an instantiated driver/interface
 * combination.  Drivers may support more than one interface,
 * and hence appear more than once in the devicetable.
 */

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <netinet/in.h>

#include <libplayercore/driver.h>
#include <libplayercore/device.h>
#include <libplayercore/message.h>
#include <libplayercore/error.h>
#include <libplayercore/playertime.h>
#include <libplayercore/globals.h>

// Constructor
Device::Device(player_devaddr_t addr, Driver *device, unsigned char access)
{
  this->next = NULL;

  this->addr = addr;
  this->driver = device;
  this->access = access;

  memset(this->drivername, 0, sizeof(this->drivername));

  if(this->driver)
  {
    this->driver->entries++;
    this->driver->device_addr = addr;
  }

  // Start with just a couple of entries; we'll double the size as
  // necessary in the future.
  this->len_queues = 2;
  this->queues = (MessageQueue**)calloc(this->len_queues, 
                                        sizeof(MessageQueue*));
  assert(this->queues);
  this->num_queues = 0;
}


Device::~Device() 
{ 
  // Shutdown and delete the associated driver
  if (this->driver) 
  {
    if (this->driver->subscriptions > 0)
      this->driver->Shutdown();
    
    // Delete only if this is the last entry for this driver
    this->driver->entries--;
    if (this->driver->entries == 0)
      delete this->driver;
  }
  free(this->queues);
}

int
Device::Subscribe(MessageQueue* sub_queue)
{
  int retval;
  size_t i;

  this->driver->Lock();
  if((retval = this->driver->Subscribe(this->addr)))
  {
    this->driver->Unlock();
    return(retval);
  }

  // add the subscriber's queue to the list

  // do we need to make room?
  if(this->num_queues == this->len_queues)
  {
    this->len_queues *= 2;
    this->queues = (MessageQueue**)realloc(this->queues, 
                                           (this->len_queues * 
                                            sizeof(MessageQueue*)));
    assert(this->queues);
  }

  // find an empty spot to put the new queue
  for(i=0;i<this->len_queues;i++)
  {
    if(!this->queues[i])
    {
      this->queues[i] = sub_queue;
      this->num_queues++;
      break;
    }
  }
  assert(i<this->len_queues);

  this->driver->Unlock();
  return(0);
}

int
Device::Unsubscribe(MessageQueue* sub_queue)
{
  int retval;

  this->driver->Lock();

  if((retval = this->driver->Unsubscribe(this->addr)))
  {
    this->driver->Unlock();
    return(retval);
  }
  // look for the given queue
  for(size_t i=0;i<this->len_queues;i++)
  {
    if(this->queues[i] == sub_queue)
    {
      this->queues[i] = NULL;
      this->num_queues--;
      this->driver->Unlock();
      return(0);
    }
  }
  PLAYER_ERROR("tried to unsubscribed not-subscribed queue");
  this->driver->Unlock();
  return(-1);
}

void
Device::PutMsg(MessageQueue* resp_queue,
               player_msghdr_t* hdr,
               void* src)
{
  hdr->addr = this->addr;
  Message msg(*hdr,src,hdr->size,resp_queue);
  // don't need to lock here, because the queue does its own locking in Push
  this->driver->InQueue->Push(msg);
}


void 
Device::PutMsg(MessageQueue* resp_queue,
               uint8_t type,
               uint8_t subtype,
               void* src,
               size_t len,
               struct timeval* timestamp)
{
  struct timeval ts;
  player_msghdr_t hdr;
  

  // Fill in the current time if not supplied
  if(timestamp)
    ts = *timestamp;
  else
    GlobalTime->GetTime(&ts);

  memset(&hdr,0,sizeof(player_msghdr_t));
  hdr.stx = PLAYER_STXX;
  hdr.type = type;
  hdr.subtype = subtype;
  hdr.timestamp_sec = ts.tv_sec;
  hdr.timestamp_usec = ts.tv_usec;
  hdr.size = len;

  this->PutMsg(resp_queue, &hdr, src);
}

