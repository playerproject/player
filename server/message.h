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

#include <player.h>
#include <pthread.h>

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
class ClientData;

class Message
{
	public:
		/// create a NULL message
		Message(); 
		/// create a new message
		Message(const struct player_msghdr & Header, const unsigned char * data,unsigned int data_size, ClientData * client = NULL); 
		/// copy pointers from existing message and increment refcounts
		Message(const Message & rhs); 
		/// destroy message, dec ref counts and delete data if ref count == 0
		~Message(); 
		
		/// GetData from message
		unsigned char * GetData() {return Data;};
		/// Size of message data
		unsigned int GetSize() {return Size;};
		
		ClientData * Client;
	private:
		unsigned char * Data;
		unsigned int * RefCount;
		unsigned int Size;
		pthread_mutex_t * Lock;
};

class MessageQueueElement
{
	public:
		MessageQueueElement();
		MessageQueueElement(MessageQueueElement & Parent, Message & Msg);
		~MessageQueueElement();
		
		Message msg;
	private:
		MessageQueueElement * prev;
		MessageQueueElement * next;
		
		friend class MessageQueue;
};

class MessageQueue
{
	public:
		MessageQueue();
		~MessageQueue();
		
		/// should we replace messages with newer ones from same device
		bool Replace;
				
		MessageQueueElement * AddMessage(Message & msg);
		MessageQueueElement * Pop();
	private:
		MessageQueueElement Head;
		MessageQueueElement * pTail;
};

#endif
