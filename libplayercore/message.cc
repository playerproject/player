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
/********************************************************************
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

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
#include <libplayercore/interface_util.h>
#include <libplayerxdr/playerxdr.h>

Message::Message(const struct player_msghdr & aHeader,
                  void * data,
                  bool copy) 
{
  CreateMessage(aHeader, data, copy);
}

Message::Message(const struct player_msghdr & aHeader,
                 void * data,
                 QueuePointer &_queue,
                 bool copy) : Queue(_queue)
{
  CreateMessage(aHeader, data, copy);
}

Message::Message(const Message & rhs)
{
  assert(rhs.Lock);
  pthread_mutex_lock(rhs.Lock);

  assert(rhs.RefCount);
  assert(*(rhs.RefCount));
  Lock = rhs.Lock;
  Data = rhs.Data;
  Header = rhs.Header;
  Queue = rhs.Queue;
  RefCount = rhs.RefCount;
  (*RefCount)++;
  ready = false;

  pthread_mutex_unlock(rhs.Lock);
}

Message::~Message()
{
  this->DecRef();
}

void Message::CreateMessage(const struct player_msghdr & aHeader,
                  void * data,
                  bool copy) 
{
  this->Lock = new pthread_mutex_t;
  assert(this->Lock);
  pthread_mutex_init(this->Lock,NULL);
  this->RefCount = new unsigned int;
  assert(this->RefCount);
  *this->RefCount = 1;

  // copy the header and then the data into out message data buffer
  memcpy(&this->Header,&aHeader,sizeof(struct player_msghdr));
  if (data == NULL)
  {
    Data = NULL;
    Header.size = 0;
    return;
  }
  // Force header size to be same as data size
  player_sizeof_fn_t sizeoffunc;
  if((sizeoffunc = playerxdr_get_sizeoffunc(Header.addr.interf, Header.type, Header.subtype)) != NULL)
  {
    Header.size = (*sizeoffunc)(data);
  }

  if (copy)
  {
    player_clone_fn_t clonefunc = NULL;
    if((clonefunc = playerxdr_get_clonefunc(Header.addr.interf, Header.type, Header.subtype)) != NULL)
    {
      if ((this->Data = (uint8_t*)(*clonefunc)(data)) == NULL)
      {
        PLAYER_ERROR3 ("failed to clone message %s: %s, %d", interf_to_str (Header.addr.interf), msgtype_to_str (Header.type), Header.subtype);
      }
    }
  }
  else
  {
    this->Data = (uint8_t*)data;
  }
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
    if (Data)
      playerxdr_free_message (Data, Header.addr.interf, Header.type, Header.subtype);
    Data = NULL;
    delete RefCount;
    RefCount = NULL;
    pthread_mutex_unlock(Lock);
    pthread_mutex_destroy(Lock);
    delete Lock;
    Lock = NULL;
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
  this->replaceRules = NULL;
  this->pull = false;
  this->data_requested = false;
}

MessageQueue::~MessageQueue()
{
  // clear the queue
  Message* msg;
  while((msg = this->Pop()))
    delete msg;

  // clear the list of replacement rules
  MessageReplaceRule* tmp;
  MessageReplaceRule* curr = this->replaceRules;
  while(curr)
  {
    tmp = curr->next;
    delete curr;
    curr = tmp;
  }

  pthread_mutex_destroy(&this->lock);
  pthread_mutex_destroy(&this->condMutex);
  pthread_cond_destroy(&this->cond);
}

/// @brief Add a replacement rule to the list
void
MessageQueue::AddReplaceRule(int _host, int _robot, int _interf, int _index,
                             int _type, int _subtype, int _replace)
{
  MessageReplaceRule* curr;
  for(curr=this->replaceRules;curr;curr=curr->next)
  {
    // Check for an existing rule with the same criteria; replace if found
    if (curr->Equivalent (_host, _robot, _interf, _index, _type, _subtype))
    {
      curr->replace = _replace;
      return;
    }
	if (curr->next == NULL)
		// Break before for() increments if this is the last one in the list
		break;
  }
  if(!curr)
  {
    curr = replaceRules = new MessageReplaceRule(_host, _robot, _interf, _index,
                                  _type, _subtype, _replace);
//	assert(curr);  // This is bad. What happens if there's a memory allocation failure when not built with debug?
    if (!curr)
      PLAYER_ERROR ("memory allocation failure; could not add new replace rule");
  }
  else
  {
    curr->next = new MessageReplaceRule(_host, _robot, _interf, _index,
                                        _type, _subtype, _replace);
//	assert(curr->next);  // This is bad. What happens if there's a memory allocation failure when not built with debug?
    if (!curr->next)
      PLAYER_ERROR ("memory allocation failure; could not add new replace rule");
  }
}

/// @brief Add a replacement rule to the list
void
MessageQueue::AddReplaceRule(const player_devaddr_t &device,
                             int _type, int _subtype, int _replace)
{
  this->AddReplaceRule (device.host, device.robot, device.interf, device.index,
                        _type, _subtype, _replace);
}

int
MessageQueue::CheckReplace(player_msghdr_t* hdr)
{
  // First look through the replacement rules
  for(MessageReplaceRule* curr=this->replaceRules;curr;curr=curr->next)
  {
  	assert(curr);
    if(curr->Match(hdr))
      return(curr->replace);
  }

  // Didn't find it; follow the default rule

  // Don't replace config requests or replies
  if((hdr->type == PLAYER_MSGTYPE_REQ) ||
     (hdr->type == PLAYER_MSGTYPE_RESP_ACK) ||
     (hdr->type == PLAYER_MSGTYPE_RESP_NACK) ||
     (hdr->type == PLAYER_MSGTYPE_SYNCH))
    return(PLAYER_PLAYER_MSG_REPLACE_RULE_ACCEPT);
  // Replace data and command according to the this->Replace flag
  else if((hdr->type == PLAYER_MSGTYPE_DATA) ||
          (hdr->type == PLAYER_MSGTYPE_CMD))
    return(this->Replace ? PLAYER_PLAYER_MSG_REPLACE_RULE_REPLACE : PLAYER_PLAYER_MSG_REPLACE_RULE_ACCEPT);
  else
  {
    PLAYER_ERROR1("encountered unknown message type %u", hdr->type);
    return(false);
  }
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

size_t
MessageQueue::GetLength(void)
{
  size_t len;
  this->Lock();
  len = this->Length;
  this->Unlock();
  return(len);
}

void MessageQueue::MarkAllReady (void)
{
  MessageQueueElement *current;
  bool dataready=false;

  if (!pull)
    return;   // No need to mark ready if not in pull mode

  Lock();
  // Mark all messages in the queue ready
  for (current = head; current != NULL; current = current->next)
  {
    player_msghdr_t* hdr;
    hdr = current->msg->GetHeader();
    // Only need to mark data and command messages.  Requests and replies
    // get marked as they are pushed in
    if((hdr->type == PLAYER_MSGTYPE_DATA) || (hdr->type == PLAYER_MSGTYPE_CMD))
    {
      current->msg->SetReady ();
      dataready=true;
    }
  }
  Unlock ();
  // Only if there was at least one message, push a sync message onto the end
  if(dataready)
  {
    struct player_msghdr syncHeader;
    syncHeader.addr.host = 0;
    syncHeader.addr.robot = 0;
    syncHeader.addr.interf = PLAYER_PLAYER_CODE;
    syncHeader.addr.index = 0;
    syncHeader.type = PLAYER_MSGTYPE_SYNCH;
    syncHeader.subtype = 0;
    Message syncMessage (syncHeader, 0, 0);
    syncMessage.SetReady ();
    this->data_requested = false;
    Push (syncMessage, true);
  }
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

bool
MessageQueue::Push(Message & msg, bool UseReserved)
{
  player_msghdr_t* hdr;

  assert(*msg.RefCount);
  this->Lock();
  hdr = msg.GetHeader();
  // Should we try to replace an older message of the same signature?
  int replaceOp = this->CheckReplace(hdr);
  if (replaceOp == PLAYER_PLAYER_MSG_REPLACE_RULE_IGNORE)
  {
    this->Unlock();
    return(true);
  }
  else if (replaceOp == PLAYER_PLAYER_MSG_REPLACE_RULE_REPLACE)
  {
    for(MessageQueueElement* el = this->tail;
        el != NULL;
        el = el->prev)
    {
      // Ignore ready flag outside pull mode - when a client goes to pull mode, only the client's
      // queue is set to pull, so other queues will still ignore ready flag
      if(el->msg->Compare(msg) && (!el->msg->Ready () || !pull))
      {
        this->Remove(el);
        delete el->msg;
        delete el;
        break;
      }
    }
  }
  // Are we over the limit?
  if(this->Length >= this->Maxlen - (UseReserved ? 0 : 1))
  {
    PLAYER_WARN("tried to push onto a full message queue");
    this->Unlock();
    if(!this->filter_on)
      this->DataAvailable();
    return(false);
  }
  else
  {
    MessageQueueElement* newelt = new MessageQueueElement();
    newelt->msg = new Message(msg);
    if (!pull || (newelt->msg->GetHeader ()->type != PLAYER_MSGTYPE_DATA &&
		  newelt->msg->GetHeader ()->type != PLAYER_MSGTYPE_CMD))
    {
      // If not in pull mode, or message is not data/cmd, set ready to true immediatly
      newelt->msg->SetReady ();
    }
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
    
    // If the client has a pending request for data, try to fulfill it
    if(this->pull && this->data_requested)
      this->MarkAllReady();
    return(true);
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

Message* MessageQueue::PopReady (void)
{
  MessageQueueElement* el;
  Lock();
  if(this->Empty())
  {
    Unlock();
    return(NULL);
  }

  // start at the head and traverse the queue until a filter-friendly
  // message (that is marked ready if in pull mode) is found
  for(el = this->head; el; el = el->next)
  {
    if((!this->filter_on || this->Filter(*el->msg)) && ((pull && el->msg->Ready ()) || !pull))
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

/// Create an empty message queue and an auto pointer to it.
QueuePointer::QueuePointer()
{
  Lock = NULL;
  RefCount = NULL;
  Queue = NULL;
}

/// Create an empty message queue and an auto pointer to it.
QueuePointer::QueuePointer(bool _Replace, size_t _Maxlen)
{
  this->Lock = new pthread_mutex_t;
  assert(this->Lock);
  pthread_mutex_init(this->Lock,NULL);
  this->Queue = new MessageQueue(_Replace, _Maxlen);
  assert(this->Queue);

  this->RefCount = new unsigned int;
  assert(this->RefCount);
  *this->RefCount = 1;		
}

/// Destroy our reference to the message queue.
/// and our queue if there are no more references
QueuePointer::~QueuePointer()
{
	DecRef();
}

/// Create a new reference to a message queue
QueuePointer::QueuePointer(const QueuePointer & rhs)
{
  if (rhs.Queue == NULL)
  {
    Lock = NULL;
    RefCount = NULL;
    Queue = NULL;      	
  }
  else
  {
    assert(rhs.Lock);
    pthread_mutex_lock(rhs.Lock);

    assert(rhs.Queue);
    assert(rhs.RefCount);
    assert(*(rhs.RefCount));
    Lock = rhs.Lock;
    Queue = rhs.Queue;
    RefCount = rhs.RefCount;
    (*RefCount)++;
    pthread_mutex_unlock(Lock);	
  }
}
	
/// assign reference to our message queue
QueuePointer & QueuePointer::operator = (const QueuePointer & rhs)
{
  // first remove our current reference
  DecRef();
  
  if (rhs.Queue == NULL)
  	return *this;
  
  // then copy the rhs
  assert(rhs.Lock);
  pthread_mutex_lock(rhs.Lock);

  assert(rhs.Queue);
  assert(rhs.RefCount);
  assert(*(rhs.RefCount));
  Lock = rhs.Lock;
  Queue = rhs.Queue;
  RefCount = rhs.RefCount;
  (*RefCount)++;
  pthread_mutex_unlock(Lock);	
  return *this;
}

/// retrieve underlying object for use
MessageQueue * QueuePointer::operator -> ()
{
  assert(Queue);
  return Queue;		
}

/// retrieve underlying object for use
MessageQueue & QueuePointer::operator * ()
{
  assert(Queue);
  return *Queue;		
}

/// check if pointers are equal
bool QueuePointer::operator == (const QueuePointer & rhs)
{
  return rhs.Queue == Queue;	
}

/// check if pointers are equal
bool QueuePointer::operator == (void * pointer)
{
  return Queue == pointer;	
}

/// check if pointers are equal
bool QueuePointer::operator != (const QueuePointer & rhs)
{
  return rhs.Queue != Queue;	
}

/// check if pointers are equal
bool QueuePointer::operator != (void * pointer)
{
  return Queue != pointer;	
}

void QueuePointer::DecRef()
{
  if (Queue == NULL)
    return;
    
  pthread_mutex_lock(Lock);
  (*RefCount)--;
  assert((*RefCount) >= 0);
  if((*RefCount)==0)
  {
    delete Queue;
    Queue = NULL;
    delete RefCount;
    RefCount = NULL;
    pthread_mutex_unlock(Lock);
    pthread_mutex_destroy(Lock);
    delete Lock;
    Lock = NULL;
  }
  else
  {
    Queue = NULL;
    RefCount = NULL;
    pthread_mutex_unlock(Lock);		
    Lock = NULL;
  }
}
