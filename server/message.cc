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

Message::Message()
{
  assert(Lock = new pthread_mutex_t);
  pthread_mutex_init(Lock,NULL);
  Size = sizeof(struct player_msghdr);
  assert(Data = new unsigned char [Size]);
  assert(RefCount = new unsigned int);
  *RefCount = 1;
  Client = NULL;
}

 
Message::Message(const struct player_msghdr & Header, 
                 const unsigned char * data, 
                 unsigned int data_size, 
                 ClientData * _client)
{
  Client = _client;
  assert(Lock = new pthread_mutex_t);
  pthread_mutex_init(Lock,NULL);
  assert(Size = sizeof(struct player_msghdr)+data_size);
  assert(Data = new unsigned char [Size]);

  // copy the header and then the data into out message data buffer
  memcpy(Data,&Header,sizeof(struct player_msghdr));
  memcpy(&Data[sizeof(struct player_msghdr)],data,data_size);
  assert(RefCount = new unsigned int);
  *RefCount = 1;
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
  pthread_mutex_lock(Lock);
  RefCount--;
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

MessageQueue::MessageQueue()
{
  Replace = false;
  pTail = &Head;
  lock = new pthread_mutex_t;
  assert(lock);
  pthread_mutex_init(lock,NULL);
}

MessageQueue::~MessageQueue()
{
  // clear the queue
  while (Pop());
}

MessageQueueElement* 
MessageQueue::Push(Message & msg)
{
  assert(pTail);
  assert(*msg.RefCount);
  Lock();
  pTail = new MessageQueueElement(*pTail,msg);
  Unlock();
  return pTail;
}

MessageQueueElement*
MessageQueue::Pop()
{
  Lock();
  if (pTail == &Head)
  {	
    Unlock();
    return NULL;
  }
  MessageQueueElement * ret = pTail;
  pTail->prev->next = NULL;
  pTail = pTail->prev;
  Unlock();
  return ret;
}
