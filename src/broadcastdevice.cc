/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: Device for inter-player communication using broadcast sockets.
// Author: Andrew Howard
// Date: 5 Feb 2000
// CVS: $Id$
//
// Theory of operation:
//  This device uses IPv4 broadcasting (not multicasting).  Be careful
//  not to run this on the USC university nets: you will get disconnected
//  and spanked!
//
///////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define PLAYER_ENABLE_TRACE 0
#include "broadcastdevice.hh"

#include <playertime.h>
extern PlayerTime* GlobalTime;


///////////////////////////////////////////////////////////////////////////
// Constructor
CBroadcastDevice::CBroadcastDevice(int argc, char** argv) :
  CDevice(0,0,0,100)
{
  this->max_queue_size = 100;
  this->addr = DEFAULT_BROADCAST_IP;
  this->port = DEFAULT_BROADCAST_PORT;
  this->read_socket = 0;
  this->write_socket = 0;

  for (int i = 0; i < argc; i++)
  {
    if (strcmp(argv[i], "addr") == 0 && i + 1 < argc)
    {
      this->addr = strdup(argv[i + 1]);
      i++;
    }
    else if (strcmp(argv[i], "port") == 0 && i + 1 < argc)
    {
      this->port = atoi(argv[i + 1]);
      i++;
    }   
    else
    {
      PLAYER_ERROR("broadcast device: invalid command line; ignoring");
      break;
    }
  }
  PLAYER_TRACE2("broadcasting on %s:%d", this->addr, this->port);
}


///////////////////////////////////////////////////////////////////////////
// Subscribe new clients to this device.  This will create a new message
// queue for each client.
int CBroadcastDevice::Subscribe(void *client)
{
  int result;
  
  // Do default subscription.
  result = CDevice::Subscribe(client);
  if (result != 0)
    return result;

  // Create a new queue
  Lock();
  AddQueue(client);
  Unlock();
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Unsubscribe clients from this device.  This will destroy the corresponding
// message queue.
int CBroadcastDevice::Unsubscribe(void *client)
{
  // Delete queue for this client
  Lock();
  DelQueue(client);
  Unlock();

  // Do default unsubscribe
  return CDevice::Unsubscribe(client);
}


///////////////////////////////////////////////////////////////////////////
// Start device
int CBroadcastDevice::Setup()
{
  PLAYER_TRACE0("initializing");
    
  // Setup the sockets
  if (SetupSockets() != 0)
    return 1;

  // Setup the message queues
  if (SetupQueues() != 0)
    return 1;

  // Start device thread
  StartThread();
  
  PLAYER_TRACE0("initializing ... done");
    
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Shutdown device
int CBroadcastDevice::Shutdown()
{
  PLAYER_TRACE0("shuting down");
  
  // Shutdown device thread
  StopThread();

  // Shutdown the message queues
  ShutdownQueues();
  
  // Shutdown the sockets
  ShutdownSockets();
  
  PLAYER_TRACE0("shutting down ... done");
    
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Handle requests.  We dont queue them up, but handle them immediately.
int CBroadcastDevice::PutConfig(player_device_id_t* id, void *client, 
                                void *data, size_t len)
{
  player_broadcast_msg_t *request;
  player_broadcast_msg_t reply;
  int replen;

  request = (player_broadcast_msg_t*) data;

  switch (request->subtype)
  {
    case PLAYER_BROADCAST_SUBTYPE_SEND:
    {
      // Write the message to the broadcast socket, and give client an ACK.
      SendPacket(request->data, len);
      if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK) != 0)
        PLAYER_ERROR("PutReply() failed");
      break;
    }
    case PLAYER_BROADCAST_SUBTYPE_RECV:
    {
      // Pop the next waiting packet from the queue and send it back
      // to the client.  If there are no waiting packets, send a NACK.
      Lock();
      replen = PopQueue(client, reply.data, sizeof(reply));
      Unlock();

      if (replen > 0)
      {
        if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL,
                     (unsigned char*) &reply, (size_t) replen) != 0)
          PLAYER_ERROR("PutReply() failed");
      }
      else
      {
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
      }
      break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void
CBroadcastDevice::Main() 
{
  int len;
  player_broadcast_msg_t msg;
  
  PLAYER_TRACE0("thread running");

  while (true)
  {
    // Get incoming messages; this is a blocking call.
    len = RecvPacket(&msg, sizeof(msg));

    // Test for thread termination; this will make the function exit
    // immediately.
    pthread_testcancel();

    // Push incoming messages on the queue.
    Lock();
    PushQueue(&msg, len);
    Unlock();
  }
}


///////////////////////////////////////////////////////////////////////////
// Setup the message queues.
// This is a list of queues (one queue for each client) implemented as a
// dynamic array of pointers to queues.
int CBroadcastDevice::SetupQueues()
{
  this->qlist_count = 0;
  this->qlist_size = 10;
  this->qlist = (queue_t**) malloc(this->qlist_size * sizeof(queue_t*));

  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Shutdown the message queues.
// First delete any queues we still have, then delete the array of
// pointers to queues.
int CBroadcastDevice::ShutdownQueues()
{
  int i;
  
  for (i = 0; i < this->qlist_count; i++)
    free(this->qlist[i]);
  this->qlist_count = 0;

  free(this->qlist);
  this->qlist_size = 0;
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Create a new queue.
// We may have to increase the size of the list.
int CBroadcastDevice::AddQueue(void *client)
{
  queue_t *queue;

  PLAYER_TRACE1("adding queue for client %p", client);

  if (this->qlist_count == this->qlist_size)
  {
    this->qlist_size *= 2;
    this->qlist = (queue_t**) realloc(this->qlist, this->qlist_size * sizeof(queue_t*));
  }
  assert(this->qlist_count < this->qlist_size);

  queue = (queue_t*) malloc(sizeof(queue_t));
  queue->client = client;
  queue->size = 10;
  queue->count = 0;
  queue->start = queue->end = 0;
  queue->msg = (void**) malloc(queue->size * sizeof(void*));
  queue->msglen = (int*) malloc(queue->size * sizeof(int));
  
  this->qlist[this->qlist_count++] = queue;
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Delete queue for a client.
// We have to find the queue, then shift all the other entries in the list.
int CBroadcastDevice::DelQueue(void *client)
{
  int index;

  PLAYER_TRACE1("deleting queue for client %p", client);
      
  index = FindQueue(client);
  if (index < 0)
  {
    PLAYER_ERROR1("queue for client %p not found", client);
    return 1;
   }

  free(this->qlist[index]);
  memmove(this->qlist + index, this->qlist + index + 1,
          (this->qlist_count - 1) * sizeof(this->qlist[0]));
  this->qlist_count--;
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Find the queue for a particular client.
int CBroadcastDevice::FindQueue(void *client)
{
  int i;

  for (i = 0; i < this->qlist_count; i++)
  {
    if (this->qlist[i]->client == client)
      return i;
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
// Push a message onto all of the queues
int CBroadcastDevice::PushQueue(void *msg, int len)
{
  int i;
  queue_t *queue;
  
  for (i = 0; i < this->qlist_count; i++)
  {
    queue = this->qlist[i];

    // Resize queue if we've run out of space.
    if (queue->count ==  queue->size)
    {
      if (queue->size >= this->max_queue_size)
        continue;
      queue->size *= 2;
      queue->msg = (void**) realloc(queue->msg, queue->size * sizeof(void*));
      queue->msglen = (int*) realloc(queue->msglen, queue->size * sizeof(int));
    }
    assert(queue->count < queue->size);

    queue->msg[queue->end] = malloc(len);
    memcpy(queue->msg[queue->end], msg, len);
    queue->msglen[queue->end] = len;
    
    queue->end = (queue->end + 1) % queue->size;
    queue->count += 1;
  }

  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Pop a message from a particular client's queue
int CBroadcastDevice::PopQueue(void *client, void *msg, int len)
{
  int index, msglen;
  queue_t *queue;
  
  index = FindQueue(client);
  if (index < 0)
  {
    PLAYER_ERROR1("queue for client %p not found", client);
    return -1;
  }
  queue = this->qlist[index];

  if (queue->count == 0)
    return 0;
  
  msglen = queue->msglen[queue->start];
  assert(msglen <= len);
  memcpy(msg, queue->msg[queue->start], msglen);
  free(queue->msg[queue->start]);

  queue->start = (queue->start + 1) % queue->size;
  queue->count -= 1;

  return msglen;
}


///////////////////////////////////////////////////////////////////////////
// Initialise the broadcast sockets
int CBroadcastDevice::SetupSockets()
{
  // Set up the write socket
  this->write_socket = socket(AF_INET, SOCK_DGRAM, 0);
  if (this->write_socket == -1)
  {
    PLAYER_ERROR1("error initializing socket : %s", strerror(errno));
    return 1;
  }
  memset(&this->write_addr, 0, sizeof(this->write_addr));
  this->write_addr.sin_family = AF_INET;
  this->write_addr.sin_addr.s_addr = inet_addr(this->addr);
  this->write_addr.sin_port = htons(this->port);

  // Set write socket options to allow broadcasting
  u_int broadcast = 1;
  if (setsockopt(this->write_socket, SOL_SOCKET, SO_BROADCAST,
                 (const char*)&broadcast, sizeof(broadcast)) < 0)
  {
    PLAYER_ERROR1("error initializing socket : %s", strerror(errno));
    return 1;
  }
    
  // Set up the read socket
  this->read_socket = socket(PF_INET, SOCK_DGRAM, 0);
  if (this->read_socket == -1)
  {
    PLAYER_ERROR1("error initializing socket : %s", strerror(errno));
    return 1;
  }

  // Set socket options to allow sharing of port
  u_int share = 1;
  if (setsockopt(this->read_socket, SOL_SOCKET, SO_REUSEADDR,
                 (const char*)&share, sizeof(share)) < 0)
  {
    PLAYER_ERROR1("error initializing socket : %s", strerror(errno));
    return 1;
  }
    
  // Bind socket to port
  memset(&this->read_addr, 0, sizeof(this->read_addr));
  this->read_addr.sin_family = AF_INET;
  this->read_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  this->read_addr.sin_port = htons(this->port);
  if (bind(this->read_socket, (sockaddr*) &this->read_addr, sizeof(this->read_addr)) < 0)
  {
    PLAYER_ERROR1("error initializing socket : %s", strerror(errno));
    return 1;
  }

  // Set socket to non-blocking
  //if (fcntl(this->read_socket, F_SETFL, O_NONBLOCK) < 0)
  //{
  //  PLAYER_ERROR1("error initializing socket : %s", strerror(errno));
  //  return 1;
  //}

  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Shutdown the broadcast sockets
int CBroadcastDevice::ShutdownSockets()
{
  // Close sockets
  close(this->write_socket);
  close(this->read_socket);

  return 0;
}



///////////////////////////////////////////////////////////////////////////
// Send a packet
void CBroadcastDevice::SendPacket(void *packet, size_t size)
{    
  if (sendto(this->write_socket, (const char*)packet, size,
             0, (sockaddr*) &this->write_addr, sizeof(this->write_addr)) < 0)
  {
    PLAYER_ERROR1("error writing to broadcast socket: %s", strerror(errno));
    return;
  }

  //PLAYER_TRACE1("sent msg len = %d", (int) size);
}


///////////////////////////////////////////////////////////////////////////
// Receive a packet
int CBroadcastDevice::RecvPacket(void *packet, size_t size)
{
  size_t addr_len = sizeof(this->read_addr);    
  int packet_len = recvfrom(this->read_socket, (char*)packet, size,
                            0, (sockaddr*) &this->read_addr, &addr_len);
  if (packet_len < 0)
  {
    PLAYER_ERROR1("error reading from broadcast socket: %s", strerror(errno));
    return 0;
  }

  //PLAYER_TRACE1("read packet len = %d", packet_len);
  return packet_len;
}







