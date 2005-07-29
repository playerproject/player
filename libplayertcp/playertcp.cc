/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005 -
 *     Brian Gerkey
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

#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <replace/replace.h>
#include <libplayercore/player.h>
#include <libplayercore/error.h>
#include <libplayerxdr/pack.h>
#include <libplayerxdr/functiontable.h>

#include "playertcp.h"
#include "socket_util.h"

PlayerTCP::PlayerTCP()
{
  this->num_clients = 0;
  this->clients = (playertcp_conn_t*)NULL;
  this->client_ufds = (struct pollfd*)NULL;

  this->num_listeners = 0;
  this->listeners = (playertcp_listener_t*)NULL;
  this->listen_ufds = (struct pollfd*)NULL;
}

PlayerTCP::~PlayerTCP()
{
  for(int i=0;i<this->num_clients;i++)
  {
    if(this->clients[i].valid)
      this->Close(i);
  }
  free(this->clients);
  free(this->listeners);
  free(this->listen_ufds);
}

int
PlayerTCP::Listen(int* ports, int num_ports)
{
  int tmp = this->num_listeners;
  this->num_listeners += num_ports;
  this->listeners = (playertcp_listener_t*)realloc(this->listeners,
                                                   this->num_listeners *
                                                   sizeof(playertcp_listener_t));
  this->listen_ufds = (struct pollfd*)realloc(this->listen_ufds,
                                              this->num_listeners *
                                              sizeof(struct pollfd));
  assert(this->listeners);
  assert(this->listen_ufds);

  for(int i=tmp;i<this->num_listeners;i++)
  {
    if((this->listeners[i].fd = 
        create_and_bind_socket(1,ports[i],PLAYER_TRANSPORT_TCP,200)) < 0)
    {
      PLAYER_ERROR("create_and_bind_socket() failed");
      return(-1);
    }
    this->listeners[i].port = ports[i];
    
    // set up for later use of poll() to accept() connections on this port
    this->listen_ufds[i].fd = this->listeners[i].fd;
    this->listen_ufds[i].events = POLLIN;
  }

  return(0);
}

int
PlayerTCP::Accept(int timeout)
{
  int num_accepts;
  int newsock;
  struct sockaddr_in cliaddr;
  socklen_t sender_len;

  // Look for new connections
  if((num_accepts = poll(this->listen_ufds, num_listeners, timeout)) < 0)
  {
    // Got interrupted by a signal; no problem
    if(errno == EINTR)
      return(0);

    // A genuine problem
    PLAYER_ERROR1("poll() failed: %s", strerror(errno));
    return(-1);
  }

  if(!num_accepts)
    return(0);

  for(int i=0; (i<num_listeners) && (i>num_accepts); i++)
  {
    if(this->listen_ufds[i].revents & POLLIN)
    {
      sender_len = sizeof(cliaddr);
      memset(&cliaddr, 0, sizeof(cliaddr));

      // Shouldn't block here
      if((newsock = accept(this->listen_ufds[i].fd, 
                           (struct sockaddr*)&cliaddr, 
                           &sender_len)) == -1)
      {
        PLAYER_ERROR1("accept() failed: %s", strerror(errno));
        return(-1);
      }

      // Look for a place to store the new connection info
      int j;
      for(j=0;j<this->num_clients;j++)
      {
        if(this->clients[j].valid)
          break;
      }

      // Do we need to allocate another spot?
      if(j == this->num_clients)
      {
        this->num_clients++;
        this->clients = (playertcp_conn_t*)realloc(this->clients,
                                                   this->num_clients *
                                                   sizeof(playertcp_conn_t));
        assert(this->clients);
        this->client_ufds = (struct pollfd*)realloc(this->client_ufds,
                                                    this->num_clients *
                                                    sizeof(struct pollfd));
        assert(this->client_ufds);
      }

      // Store the client's info
      this->clients[j].valid = 1;
      this->clients[j].fd = newsock;
      this->clients[j].addr = cliaddr;

      // Set up for later use of poll
      this->client_ufds[j].fd = this->clients[j].fd;


      // Create an outgoing queue for this client
      this->clients[j].queue = 
              new MessageQueue(0,PLAYER_MSGQUEUE_DEFAULT_MAXLEN);
      assert(this->clients[j].queue);

      // Create a buffer to hold incoming messages
      this->clients[j].readbuffersize = PLAYERTCP_READBUFFER_SIZE;
      this->clients[j].readbuffer = 
              (char*)calloc(1,this->clients[j].readbuffersize);
      assert(this->clients[j].readbuffer);
      this->clients[j].readbufferlen = 0;

      num_accepts--;
    }
  }

  return(0);
}

void
PlayerTCP::Close(int cli)
{
  assert((cli >= 0) && (cli < this->num_clients));

  if(!this->clients[cli].valid)
    PLAYER_WARN1("tried to Close() invalid client connection %d", cli);
  else
  {
    if(close(this->clients[cli].fd) < 0)
      PLAYER_WARN1("close() failed: %s", strerror(errno));
    this->clients[cli].fd = -1;
    this->clients[cli].valid = 0;
    delete this->clients[cli].queue;
    free(this->clients[cli].readbuffer);
  }
}

int
PlayerTCP::ReadClient(int cli)
{
  int numread;
  playertcp_conn_t* client;

  assert((cli >= 0) && (cli < this->num_clients));

  client = this->clients + cli;

  // Read until there's nothing left to read.
  for(;;)
  {
    // Might we need more room to assemble the current partial message?
    if((client->readbuffersize - client->readbufferlen) < 
       PLAYERTCP_READBUFFER_SIZE)
    {
      // Get twice as much space.
      client->readbuffersize *= 2;
      // Did we hit the limit (or overflow and become negative)?
      if((client->readbuffersize >= PLAYERTCP_MAX_MESSAGE_LEN) ||
         (client->readbuffersize < 0))
      {
        PLAYER_WARN2("allocating maximum %d bytes to client %d's read buffer",
                    PLAYERTCP_MAX_MESSAGE_LEN, cli);
        client->readbuffersize = PLAYERTCP_MAX_MESSAGE_LEN;
      }
      client->readbuffer = (char*)realloc(client->readbuffer, 
                                          client->readbuffersize);
      assert(client->readbuffer);
      memset(client->readbuffer + client->readbufferlen, 0, 
             client->readbuffersize - client->readbufferlen);
    }

    // Having allocated more space, are we full?
    if(client->readbuffersize == client->readbufferlen)
    {
      PLAYER_WARN2("client %d's buffer is full (%d bytes)",
                   cli, client->readbuffersize);
      break;
    }

    numread = read(client->fd, 
                   client->readbuffer + client->readbufferlen,
                   client->readbuffersize - client->readbufferlen);

    if(numread < 1)
    {
      if(errno == EAGAIN)
      {
        // No more data available.
        break;
      }
      else
      {
        PLAYER_ERROR1("read() failed: %s", strerror(errno));
        return(-1);
      }
    }
  }

  // Try to parse the data received so far
  this->ParseBuffer(cli);
  return(0);
}

void
PlayerTCP::ParseBuffer(int cli)
{
  player_msghdr_t hdr;
  playertcp_conn_t* client;
  //Message* msg;
  player_pack_fn_t packfunc;
  int headerlen;
  int msglen;

  assert((cli >= 0) && (cli < this->num_clients));
  client = this->clients + cli;

  // Process one message in each iteration
  for(;;)
  {
    // Try to read the header
    if((headerlen = player_msghdr_pack(client->readbuffer, 
                                       client->readbufferlen,
                                       &hdr, PLAYERXDR_DECODE)) < 0)
    {
      // Failed to read the header; presumably not enough bytes received yet.
      return;
    }

    msglen = headerlen + hdr.size;

    // Is the message of a legal size?
    if(msglen > PLAYERTCP_MAX_MESSAGE_LEN)
    {
      PLAYER_WARN2("incoming message is larger than max (%d > %d); truncating",
                   msglen, PLAYERTCP_MAX_MESSAGE_LEN);
      msglen = PLAYERTCP_MAX_MESSAGE_LEN;
    }

    // Is it all here yet?
    if(msglen > client->readbufferlen)
      return;

    // Locate the appropriate packing function
    if(!(packfunc = playerxdr_get_func(hdr.addr.interface, hdr.subtype)))
    {
      // TODO: Allow the user to register a callback to handle unsupported
      // messages
      PLAYER_WARN3("skipping message to %u:%u with unsupported type %u",
                   hdr.addr.interface, hdr.addr.index, hdr.subtype);
    }
    else
    {
      /*
      if((*packfunc)(client->readbuffer + client->readbufferlen,
                     client->readbuffersize - client->readbufferlen,
                     (void*)&
                     */
    }

    // Move past the processed message
    memmove(client->readbuffer, client->readbuffer + msglen, msglen);
    client->readbufferlen -= msglen;
  }
}


