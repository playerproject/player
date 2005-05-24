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

#ifndef MESSAGE_H
#define MESSAGE_H

#include <pthread.h>

#include <libplayercore/player.h>

// TODO: remove this dependency
class ClientData;

/**
The message class provides reference counting for a message that is 
to be delivered to several clients. This should save some data copying
and hopefully limit the speed overhead.

If these objects are going to be accessed from more than one thread
then locking should be added in the constructor/destructors
to make sure the reference counting is honoured correctly.

Drivers should have access to a create message function
which will take header, data and target client(can be set to all subscribed). 
This function will then create the message object and put it on the 
appropriate client queue/queues
*/
class Message
{
  public:
    /// Create a NULL message.
    Message(); 
    /// Create a new message.
    Message(const struct player_msghdr & Header, 
            const unsigned char * data,
            unsigned int data_size, 
            ClientData * client = NULL); 
    /// Copy pointers from existing message and increment refcount.
    Message(const Message & rhs); 

    /// Destroy message, dec ref counts and delete data if ref count == 0
    ~Message(); 

    /// GetData from message.
    unsigned char * GetData() {return Data;};
    /// Get pointer to header.
    player_msghdr_t * GetHeader() {return reinterpret_cast<player_msghdr_t *> (Data);};
    /// Get pointer to payload.
    uint8_t * GetPayload() {return (&Data[sizeof(player_msghdr_t)]);};
    /// Get Payload size.
    size_t GetPayloadSize() {return Size - sizeof(player_msghdr_t);};
    /// Size of message data.
    unsigned int GetSize() {return Size;};
    /// Compare type, subtype, device, and device_index.
    bool Compare(Message &other);
    /// Decrement ref count
    void DecRef();

    /// ???
    ClientData* Client;

    /// Reference count.
    unsigned int * RefCount;
  private:
    /// Pointer to the message data.
    uint8_t * Data;
    /// Length (in bytes) of Data.
    unsigned int Size;
    /// Used to lock access to Data.
    pthread_mutex_t * Lock;
};

/**
 This class is a helper for maintaining doubly-linked queues of Messages.
*/
class MessageQueueElement
{
  public:
    /// Create a queue element with NULL prev and next pointers.
    MessageQueueElement();
    /// Create a queue element with prev pointing to Parent and next
    /// pointing to Parent.next. I.e., insert Msg after Parent.
    MessageQueueElement(MessageQueueElement & Parent, Message & Msg);
    /// Destroy a queue element.
    ~MessageQueueElement();

    /// The message stored in this queue element.
    Message msg;
  private:
    /// Pointer to previous queue element.
    MessageQueueElement * prev;
    /// Pointer to next queue element.
    MessageQueueElement * next;

    friend class MessageQueue;
};

/**
 A doubly-linked queue of messages.
*/
class MessageQueue
{
  public:
    /// Create an empty message queue.
    MessageQueue(bool _Replace, size_t _Maxlen);
    /// Destroy a message queue.
    ~MessageQueue();
    /// Push a message onto the queue.  Returns a pointer to the new last
    /// element in the queue.
    MessageQueueElement * Push(Message & msg);
    /// Pop message element @p el from the queue.  If @p el is NULL, then the 
    /// tail of the queue is popped.  Returns pointer to said message, or
    /// NULL if the queue is empty.
    MessageQueueElement * Pop(MessageQueueElement* el = NULL);
    /// Set the @p Replace flag, which governs whether data and command
    /// messages of the same subtype from the same device are replaced in
    /// the queue.
    void SetReplace(bool _Replace) { this->Replace = _Replace; };
  private:
    /// Lock the mutex associated with this queue.
    void Lock() {pthread_mutex_lock(lock);};
    /// Unlock the mutex associated with this queue.
    void Unlock() {pthread_mutex_unlock(lock);};
    /// Remove element @p el from the queue, and rearrange pointers
    /// appropriately.
    void Remove(MessageQueueElement* el);
    /// Head of the queue.
    MessageQueueElement Head;
    /// Tail of the queue.
    MessageQueueElement * pTail;
    /// Mutex to control access to the queue, via Lock() and Unlock().
    pthread_mutex_t * lock;
    /// Maximum length of queue in elements.
    size_t Maxlen;
    /// Should we replace messages with newer ones from same device?
    bool Replace;
    /// Current length of queue, in elements.
    size_t Length;
};

#endif
