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
 * Desc: Message class and message queues
 * CVS:  $Id$
 * Author: Toby Collett - Jan 2005
 */
 
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "message.h"
#include "player.h"
#include "error.h"

Message::Message()
{
  this->Lock = new pthread_mutex_t;
  assert(Lock);
  pthread_mutex_init(this->Lock,NULL);
  this->Size = sizeof(struct player_msghdr);
  this->Data = new unsigned char [this->Size];
  assert(Data);
  memset(this->Data,0,this->Size);
  this->RefCount = new unsigned int;
  assert(RefCount);
  *this->RefCount = 1;
  this->Client = NULL;
}

 
Message::Message(const struct player_msghdr & Header, 
                 const unsigned char * data, 
                 unsigned int data_size, 
                 ClientData * _client)
{
  Client = _client;
  this->Lock = new pthread_mutex_t;
  assert(Lock);
  pthread_mutex_init(this->Lock,NULL);
  Size = sizeof(struct player_msghdr)+data_size;
  assert(Size);
  Data = new unsigned char[Size];
  assert(Data);

  // copy the header and then the data into out message data buffer
  memcpy(this->Data,&Header,sizeof(struct player_msghdr));
  memcpy(&this->Data[sizeof(struct player_msghdr)],data,data_size);
  this->RefCount = new unsigned int;
  assert(RefCount);
  *this->RefCount = 1;
}

Message::Message(const Message & rhs)
{
  Client = rhs.Client;
  assert(rhs.Lock);
  pthread_mutex_lock(rhs.Lock);
  assert(rhs.Data);
  assert(rhs.RefCount);
  assert(*(rhs.RefCount));
  Lock=rhs.Lock;
  Data=rhs.Data;
  Size = rhs.Size;
  RefCount = rhs.RefCount;
  (*RefCount)++;
  pthread_mutex_unlock(Lock);
}

Message::~Message()
{
  this->DecRef();
}

bool 
Message::Compare(Message &other)
{ 
  player_msghdr_t* thishdr = this->GetHeader();
  player_msghdr_t* otherhdr = other.GetHeader();
  return((thishdr->type == otherhdr->type) &&
         (thishdr->subtype == otherhdr->subtype) &&
         (thishdr->device == otherhdr->device) &&
         (thishdr->device_index == otherhdr->device_index));
};

void 
Message::DecRef()
{
  pthread_mutex_lock(Lock);
  RefCount--;
  assert(RefCount >= 0);
  if(RefCount==0)
  {
    delete [] Data;
    delete RefCount;
    pthread_mutex_unlock(Lock);
    pthread_mutex_destroy(Lock);
    delete Lock;
  }
  else
    pthread_mutex_unlock(Lock);
}

MessageQueueElement::MessageQueueElement()
{
  prev = NULL;
  next = NULL;
}

MessageQueueElement::MessageQueueElement(MessageQueueElement & Parent, 
                                         Message & Msg) : msg(Msg)
{
  assert(*(msg.RefCount));
  prev = &Parent;
  next = Parent.next;
  Parent.next = this;
}

MessageQueueElement::~MessageQueueElement()
{
}

MessageQueue::MessageQueue(bool _Replace, size_t _Maxlen)
{
  this->Replace = _Replace;
  this->Maxlen = _Maxlen;
  this->pTail = &this->Head;
  this->lock = new pthread_mutex_t;
  this->Length = 0;
  assert(this->lock);
  pthread_mutex_init(this->lock,NULL);
}

MessageQueue::~MessageQueue()
{
  // clear the queue
  while(Pop());
}

MessageQueueElement* 
MessageQueue::Push(Message & msg)
{
  player_msghdr_t* hdr;
  assert(this->pTail);
  assert(*msg.RefCount);
  this->Lock();
  hdr = msg.GetHeader();
  if(this->Replace && 
     ((hdr->type == PLAYER_MSGTYPE_DATA) || 
      (hdr->type == PLAYER_MSGTYPE_CMD)))
  {
    for(MessageQueueElement* el = this->pTail; 
        el != &this->Head; 
        el = el->prev)
    {
      if(el->msg.Compare(msg))
      {
        this->Remove(el);
        delete el;
        break;
      }
    }
  }
  if(this->Length >= this->Maxlen)
  {
    PLAYER_WARN("tried to push onto a full message queue");
    this->Unlock();
    return(NULL);
  }
  else
  {
    this->pTail = new MessageQueueElement(*this->pTail,msg);
    this->Length++;
    this->Unlock();
    return(this->pTail);
  }
}

MessageQueueElement*
MessageQueue::Pop(MessageQueueElement* el)
{
  Lock();
  if(pTail == &Head)
  {	
    Unlock();
    return NULL;
  }
  if(!el)
  {
  	el = this->Head.next;
    assert(el);
  }
  this->Remove(el);
  Unlock();
  return(el);
}

void
MessageQueue::Remove(MessageQueueElement* el)
{
  el->prev->next = el->next;
  if(el == this->pTail)
    this->pTail = this->pTail->prev;
  else
    el->next->prev = el->prev;
  this->Length--;
}

