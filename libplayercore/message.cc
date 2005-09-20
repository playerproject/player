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

#include <libplayercore/message.h>
#include <libplayercore/player.h>
#include <libplayercore/error.h>

Message::Message(const struct player_msghdr & Header, 
                 const void * data, 
                 unsigned int data_size, 
                 MessageQueue* _queue)
{
  this->Queue = _queue;
  this->Lock = new pthread_mutex_t;
  assert(this->Lock);
  pthread_mutex_init(this->Lock,NULL);
  this->Size = sizeof(struct player_msghdr)+data_size;
  assert(this->Size);
  this->Data = new unsigned char[this->Size];
  assert(this->Data);

  // copy the header and then the data into out message data buffer
  memcpy(this->Data,&Header,sizeof(struct player_msghdr));
  // Force header size to be same as data size
  ((player_msghdr *) Data)->size = data_size;

  memcpy(&this->Data[sizeof(struct player_msghdr)],data,data_size);
  this->RefCount = new unsigned int;
  assert(this->RefCount);
  *this->RefCount = 1;
}

Message::Message(const Message & rhs)
{
  assert(rhs.Lock);
  pthread_mutex_lock(rhs.Lock);

  assert(rhs.Data);
  assert(rhs.RefCount);
  assert(*(rhs.RefCount));
  Lock = rhs.Lock;
  Data = rhs.Data;
  Size = rhs.Size;
  Queue = rhs.Queue;
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
  return(Message::MatchMessage(thishdr,
                               otherhdr->type,
                               otherhdr->subtype,
                               otherhdr->addr));
};

void 
Message::DecRef()
{
  pthread_mutex_lock(Lock);
  (*RefCount)--;
  assert((*RefCount) >= 0);
  if((*RefCount)==0)
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
  msg = NULL;
  prev = next = NULL;
}

MessageQueueElement::~MessageQueueElement()
{
}

MessageQueue::MessageQueue(bool _Replace, size_t _Maxlen)
{
  this->Replace = _Replace;
  this->Maxlen = _Maxlen;
  this->head = this->tail = NULL;
  this->Length = 0;
  pthread_mutex_init(&this->lock,NULL);
  pthread_mutex_init(&this->condMutex,NULL);
  pthread_cond_init(&this->cond,NULL);
  this->ClearFilter();
  this->filter_on = false;
}

MessageQueue::~MessageQueue()
{
  // clear the queue
  while(this->Pop());
  pthread_mutex_destroy(&this->lock);
  pthread_mutex_destroy(&this->condMutex);
  pthread_cond_destroy(&this->cond);
}

// Waits on the condition variable associated with this queue.
void 
MessageQueue::Wait(void)
{
  MessageQueueElement* el;

  // don't wait if there's data on the queue
  this->Lock();
  // start at the head and traverse the queue until a filter-friendly
  // message is found
  for(el = this->head; el; el = el->next)
  {
    if(!this->filter_on || this->Filter(*el->msg))
      break;
  }
  this->Unlock();
  if(el)
    return;

  // need to push this cleanup function, cause if a thread is cancelled while
  // in pthread_cond_wait(), it will immediately relock the mutex.  thus we
  // need to unlock ourselves before exiting.
  pthread_cleanup_push((void(*)(void*))pthread_mutex_unlock,
                       (void*)&this->condMutex);
  pthread_mutex_lock(&this->condMutex);
  pthread_cond_wait(&this->cond,&this->condMutex);
  pthread_mutex_unlock(&this->condMutex);
  pthread_cleanup_pop(0);
}

bool
MessageQueue::Filter(Message& msg)
{
  player_msghdr_t* hdr = msg.GetHeader();
  return(((this->filter_host < 0) || 
          ((unsigned int)this->filter_host == hdr->addr.host)) &&
         ((this->filter_robot < 0) || 
          ((unsigned int)this->filter_robot == hdr->addr.robot)) &&
         ((this->filter_interf < 0) || 
          ((unsigned int)this->filter_interf == hdr->addr.interf)) &&
         ((this->filter_index < 0) || 
          ((unsigned int)this->filter_index == hdr->addr.index)) &&
         (((this->filter_type < 0) && 
           ((hdr->type == PLAYER_MSGTYPE_RESP_ACK) ||
            (hdr->type == PLAYER_MSGTYPE_RESP_NACK))) || 
          ((unsigned int)this->filter_type == hdr->type)) &&
         ((this->filter_subtype < 0) || 
          ((unsigned int)this->filter_subtype == hdr->subtype)));
}

void
MessageQueue::SetFilter(int host, int robot, int interf, 
                        int index, int type, int subtype)
{
  this->filter_host = host;
  this->filter_robot = robot;
  this->filter_interf = interf;
  this->filter_index = index;
  this->filter_type = type;
  this->filter_subtype = subtype;
  this->filter_on = true;
}

void
MessageQueue::ClearFilter(void)
{
  this->filter_on = false;
}


// Signal that new data is available (calls pthread_cond_broadcast()
// on this device's condition variable, which will release other
// devices that are waiting on this one).
void 
MessageQueue::DataAvailable(void)
{
  pthread_mutex_lock(&this->condMutex);
  pthread_cond_broadcast(&this->cond);
  pthread_mutex_unlock(&this->condMutex);
}

MessageQueueElement* 
MessageQueue::Push(Message & msg)
{
  player_msghdr_t* hdr;

  assert(*msg.RefCount);
  this->Lock();
  hdr = msg.GetHeader();
  if(this->Replace && 
     ((hdr->type == PLAYER_MSGTYPE_DATA) || 
      (hdr->type == PLAYER_MSGTYPE_CMD)))
  {
    for(MessageQueueElement* el = this->tail; 
        el != NULL;
        el = el->prev)
    {
      if(el->msg->Compare(msg))
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
    if(!this->filter_on)
      this->DataAvailable();
    return(NULL);
  }
  else
  {
    MessageQueueElement* newelt = new MessageQueueElement();
    newelt->msg = new Message(msg);
    if(!this->tail)
    {
      this->head = this->tail = newelt;
      newelt->prev = newelt->next = NULL;
    }
    else
    {
      this->tail->next = newelt;
      newelt->prev = this->tail;
      newelt->next = NULL;
      this->tail = newelt;
    }
    this->Length++;
    this->Unlock();
    if(!this->filter_on || this->Filter(msg))
      this->DataAvailable();
    return(newelt);
  }
}

Message*
MessageQueue::Pop()
{
  MessageQueueElement* el;
  Lock();
  if(this->Empty())
  {
    Unlock();
    return(NULL);
  }

  // start at the head and traverse the queue until a filter-friendly
  // message is found
  for(el = this->head; el; el = el->next)
  {
    if(!this->filter_on || this->Filter(*el->msg))
    {
      this->Remove(el);
      Unlock();
      Message* retmsg = el->msg;
      delete el;
      return(retmsg);
    }
  }
  Unlock();
  return(NULL);
}

void
MessageQueue::Remove(MessageQueueElement* el)
{
  if(el->prev)
    el->prev->next = el->next;
  else
    this->head = el->next;
  if(el->next)
    el->next->prev = el->prev;
  else
    this->tail = el->prev;
  this->Length--;
}

