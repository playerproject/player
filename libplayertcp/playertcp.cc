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
#include <fcntl.h>

#include <replace/replace.h>
#include <libplayercore/playercore.h>
#include <libplayerxdr/playerxdr.h>

#include "playertcp.h"
#include "socket_util.h"

PlayerTCP::PlayerTCP()
{
  this->size_clients = 0;
  this->num_clients = 0;
  this->clients = (playertcp_conn_t*)NULL;
  this->client_ufds = (struct pollfd*)NULL;

  this->num_listeners = 0;
  this->listeners = (playertcp_listener_t*)NULL;
  this->listen_ufds = (struct pollfd*)NULL;

  // Create a buffer to hold decoded incoming messages
  this->decode_readbuffersize = PLAYERTCP_READBUFFER_SIZE;
  this->decode_readbuffer = 
          (char*)calloc(1,this->decode_readbuffersize);
  assert(this->decode_readbuffer);
}

PlayerTCP::~PlayerTCP()
{
  for(int i=0;i<this->num_clients;i++)
    this->Close(i);
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
  unsigned char data[PLAYER_IDENT_STRLEN];

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

  for(int i=0; (i<num_listeners) && (num_accepts>0); i++)
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

      // make the socket non-blocking
      if(fcntl(newsock, F_SETFL, O_NONBLOCK) == -1)
      {
        PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
        close(newsock);
        return(-1);
      }

      // Look for a place to store the new connection info
      int j;
      for(j=0;j<this->size_clients;j++)
      {
        if(!this->clients[j].valid)
          break;
      }

      // Do we need to allocate another spot?
      if(j == this->size_clients)
      {
        this->size_clients++;
        this->clients = (playertcp_conn_t*)realloc(this->clients,
                                                   this->size_clients *
                                                   sizeof(playertcp_conn_t));
        assert(this->clients);
        this->client_ufds = (struct pollfd*)realloc(this->client_ufds,
                                                    this->size_clients *
                                                    sizeof(struct pollfd));
        assert(this->client_ufds);
      }

      // Store the client's info
      this->clients[j].valid = 1;
      this->clients[j].del = 0;
      this->clients[j].port = this->listeners[i].port;
      this->clients[j].fd = newsock;
      this->clients[j].addr = cliaddr;

      // Set up for later use of poll
      this->client_ufds[j].fd = this->clients[j].fd;
      this->client_ufds[j].events = POLLIN;

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

      // Create a buffer to hold outgoing messages
      this->clients[j].writebuffersize = PLAYERTCP_WRITEBUFFER_SIZE;
      this->clients[j].writebuffer = 
              (char*)calloc(1,this->clients[j].writebuffersize);
      assert(this->clients[j].writebuffer);
      this->clients[j].writebufferlen = 0;

      sprintf((char*)data, "%s", PLAYER_IDENT_STRING);
      memset(((char*)data)+strlen((char*)data),0,
             PLAYER_IDENT_STRLEN-strlen((char*)data));
      if(write(this->clients[j].fd, (void*)data, PLAYER_IDENT_STRLEN) < 0)
      {
        PLAYER_ERROR("failed to send ident string");
      }

      PLAYER_MSG3(1, "accepted client %d on port %d, fd %d",
                  j, this->clients[j].port, this->clients[j].fd);

      this->num_clients++;
      num_accepts--;
    }
  }

  return(0);
}

void
PlayerTCP::Close(int cli)
{
  assert((cli >= 0) && (cli < this->num_clients));

  PLAYER_MSG2(1, "closing connection to client %d on port %d",
              cli, this->clients[cli].port);

  if(!this->clients[cli].valid)
    PLAYER_WARN1("tried to Close() invalid client connection %d", cli);
  else
  {
    if(close(this->clients[cli].fd) < 0)
      PLAYER_WARN1("close() failed: %s", strerror(errno));
    this->clients[cli].fd = -1;
    this->clients[cli].valid = 0;
    this->clients[cli].del = 0;
    delete this->clients[cli].queue;
    free(this->clients[cli].readbuffer);
    free(this->clients[cli].writebuffer);
  }
}

int
PlayerTCP::Read(int timeout)
{
  int num_available;

  // Poll for incoming messages
  if((num_available = poll(this->client_ufds, this->num_clients, timeout)) < 0)
  {
    // Got interrupted by a signal; no problem
    if(errno == EINTR)
      return(0);

    // A genuine problem
    PLAYER_ERROR1("poll() failed: %s", strerror(errno));
    return(-1);
  }

  if(!num_available)
    return(0);

  for(int i=0; (i<this->num_clients) && (num_available>0); i++)
  {
    if((this->clients[i].valid) && 
       ((this->client_ufds[i].revents & POLLERR) || 
        (this->client_ufds[i].revents & POLLHUP) ||
        (this->client_ufds[i].revents & POLLNVAL)))
    {
      PLAYER_WARN1("other error on client %d", i);
      this->clients[i].del = 1;
      num_available--;
    }
    else if((this->clients[i].valid) && 
            (this->client_ufds[i].revents & POLLIN))
    {
      if(this->ReadClient(i) < 0)
      {
        PLAYER_WARN1("failed to read from client %d", i);
        this->clients[i].del = 1;
      }
      num_available--;
    }
  }

  this->DeleteClients();

  return(0);
}

void
PlayerTCP::DeleteClients()
{
  // Delete those connections that generated errors in this iteration
  for(int i=0; i<this->size_clients; i++)
  {
    if(this->clients[i].del)
    {
      this->Close(i);
      // Remove the resulting blank from both lists
      memmove(this->clients + i, 
              this->clients + i + 1, 
              (this->size_clients - i - 1) * sizeof(playertcp_conn_t));
      memmove(this->client_ufds + i, 
              this->client_ufds + i + 1, 
              (this->size_clients - i - 1) * sizeof(struct pollfd));
      this->num_clients--;
      i--;
    }
  }
  memset(this->clients + this->num_clients, 0,
         (this->size_clients - this->num_clients) * sizeof(playertcp_conn_t));
  memset(this->client_ufds + this->num_clients, 0,
         (this->size_clients - this->num_clients) * sizeof(struct pollfd));
}

int
PlayerTCP::WriteClient(int cli)
{
  int numwritten;
  playertcp_conn_t* client;
  Message* msg;
  player_pack_fn_t packfunc;
  player_msghdr_t* hdr;
  void* payload;
  int encode_msglen;

  client = this->clients + cli;
  for(;;)
  {
    // try to send any bytes leftover from last time.
    if(client->writebufferlen)
    {
      numwritten = write(client->fd, 
                         client->writebuffer, 
                         MIN(client->writebufferlen,
                             PLAYERTCP_WRITEBUFFER_SIZE));

      if(numwritten < 0)
      {
        if(errno == EAGAIN)
        {
          // buffers are full
          return(0);
        }
        else
        {
          PLAYER_MSG1(2,"read() failed: %s", strerror(errno));
          return(-1);
        }
      }
      else if(numwritten == 0)
      {
        PLAYER_MSG0(2,"wrote zero bytes");
        return(-1);
      }
      
      memmove(client->writebuffer, client->writebuffer + numwritten,
              client->writebufferlen - numwritten);
      client->writebufferlen -= numwritten;
    }
    // try to pop a pending message
    else if((msg = client->queue->Pop()))
    {
      hdr = msg->GetHeader();
      payload = msg->GetPayload();
      // Locate the appropriate packing function
      if(!(packfunc = playerxdr_get_func(hdr->addr.interf, hdr->subtype)))
      {
        // TODO: Allow the user to register a callback to handle unsupported
        // messages
        PLAYER_WARN3("skipping message from %u:%u with unsupported type %u",
                     hdr->addr.interf, hdr->addr.index, hdr->subtype);
      }
      else
      {
        // Make sure there's room in the buffer for the encoded messsage.
        // 4 times the message is a safe upper bound
        if((4* hdr->size) > (size_t)(client->writebuffersize))
        {
          // Get at least twice as much space
          client->writebuffersize = MAX((size_t)(client->writebuffersize * 2), 
                                        4*hdr->size);
          // Did we hit the limit (or overflow and become negative)?
          if((client->writebuffersize >= PLAYERTCP_MAX_MESSAGE_LEN) ||
             (client->writebuffersize < 0))
          {
            PLAYER_WARN1("allocating maximum %d bytes to outgoing message buffer",
                         PLAYERTCP_MAX_MESSAGE_LEN);
            client->writebuffersize = PLAYERTCP_MAX_MESSAGE_LEN;
          }
          client->writebuffer = (char*)realloc(client->writebuffer,
                                               client->writebuffersize);
          assert(client->writebuffer);
          memset(client->writebuffer, 0, client->writebuffersize);
        }

        // Encode the body first
        if((encode_msglen =
            (*packfunc)(client->writebuffer + PLAYERXDR_MSGHDR_SIZE,
                        client->writebuffersize - PLAYERXDR_MSGHDR_SIZE,
                        payload, PLAYERXDR_ENCODE)) < 0)
        {
          PLAYER_WARN3("encoding failed on message from %u:%u with type %u",
                       hdr->addr.interf, hdr->addr.index, hdr->subtype);
          client->writebufferlen = 0;
          return(0);
        }

        // Rewrite the size in the header with the length of the encoded
        // body, then encode the header.
        hdr->size = encode_msglen;
	if((encode_msglen =
	    player_msghdr_pack(client->writebuffer,
			       client->writebuffersize, hdr,
			       PLAYERXDR_ENCODE)) < 0)
        {
          PLAYER_ERROR("failed to encode msg header");
          client->writebufferlen = 0;
          return(0);
        }

        client->writebufferlen = PLAYERXDR_MSGHDR_SIZE + hdr->size;
      }
    }
    else
      return(0);
  }
}

int
PlayerTCP::Write()
{
  for(int i=0;i<this->num_clients;i++)
  {
    if(this->WriteClient(i) < 0)
    {
      PLAYER_WARN1("failed to write to client %d\n", i);
      this->clients[i].del = 1;
    }
  }

  this->DeleteClients();
  return(0);
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

    if(numread < 0)
    {
      if(errno == EAGAIN)
      {
        // No more data available.
        break;
      }
      else
      {
        PLAYER_MSG1(2,"read() failed: %s", strerror(errno));
        return(-1);
      }
    }
    else if(numread == 0)
    {
      PLAYER_MSG0(2, "read() read zero bytes");
      return(-1);
    }
    else
      client->readbufferlen += numread;
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
  player_pack_fn_t packfunc;
  int headerlen;
  int msglen;
  int decode_msglen;
  Device* device;

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

    device = deviceTable->GetDevice(hdr.addr);
    if(!device && (hdr.addr.interf != PLAYER_PLAYER_CODE))
    {
      PLAYER_WARN3("skipping message of type %u to unknown device %u:%u",
                   hdr.subtype, hdr.addr.interf, hdr.addr.index);
    }
    else
    {
      // Locate the appropriate packing function
      if(!(packfunc = playerxdr_get_func(hdr.addr.interf, hdr.subtype)))
      {
        // TODO: Allow the user to register a callback to handle unsupported
        // messages
        PLAYER_WARN3("skipping message to %u:%u with unsupported type %u",
                     hdr.addr.interf, hdr.addr.index, hdr.subtype);
      }
      else
      {
        // Make sure there's room in the buffer for the decoded messsage.
        // Because XDR can only inflate the size of a message, the encoded
        // length is an upper bound on the decoded length.
        if(this->decode_readbuffersize < msglen)
        {
          // Get at least twice as much space
          this->decode_readbuffersize = 
                  MAX(this->decode_readbuffersize * 2, msglen);
          // Did we hit the limit (or overflow and become negative)?
          if((this->decode_readbuffersize >= PLAYERTCP_MAX_MESSAGE_LEN) ||
             (this->decode_readbuffersize < 0))
          {
            PLAYER_WARN1("allocating maximum %d bytes to decoded message buffer",
                         PLAYERTCP_MAX_MESSAGE_LEN);
            this->decode_readbuffersize = PLAYERTCP_MAX_MESSAGE_LEN;
          }
          this->decode_readbuffer = (char*)realloc(this->decode_readbuffer,
                                                   this->decode_readbuffersize);
          assert(this->decode_readbuffer);
          memset(this->decode_readbuffer, 0, this->decode_readbuffersize);
        }

	if((decode_msglen =
	    (*packfunc)(client->readbuffer + PLAYERXDR_MSGHDR_SIZE,
			client->readbufferlen - PLAYERXDR_MSGHDR_SIZE,
                        (void*)this->decode_readbuffer,
                        PLAYERXDR_DECODE)) < 0)
        {
          PLAYER_WARN3("decoding failed on message to %u:%u with type %u",
                       hdr.addr.interf, hdr.addr.index, hdr.subtype);
        }
        else
        {
          // update the message size and send it off
          hdr.size = decode_msglen;
          if(hdr.addr.interf == PLAYER_PLAYER_CODE)
          {
            Message* msg = new Message(hdr, this->decode_readbuffer,
                                       hdr.size, client->queue);
            assert(msg);
            this->HandlePlayerMessage(cli, msg);
            delete msg;
          }
          else
            device->PutMsg(client->queue, &hdr, this->decode_readbuffer);
        }
      }
    }

    // Move past the processed message
    memmove(client->readbuffer, client->readbuffer + msglen, msglen);
    client->readbufferlen -= msglen;
  }
}

int
PlayerTCP::HandlePlayerMessage(int cli, Message* msg)
{
  player_msghdr_t* hdr;
  void* payload;
  player_device_req_t* devreq;
  Device* device;
  playertcp_conn_t* client;

  player_device_resp_t devresp;
  player_msghdr_t resphdr;
  Message* resp;

  assert((cli >= 0) && (cli < this->num_clients));
  client = this->clients + cli;

  hdr = msg->GetHeader();
  payload = msg->GetPayload();

  resphdr = *hdr;

  switch(hdr->type)
  {
    case PLAYER_MSGTYPE_REQ:
      switch(hdr->subtype)
      {
        // Device subscription request
        case PLAYER_PLAYER_DEV:
          devreq = (player_device_req_t*)payload;

          memset(&devresp,0,sizeof(player_device_resp_t));
          devresp.addr = devreq->addr;
          devresp.access = PLAYER_ERROR_MODE;
          devresp.driver_name_count = 0;

          resphdr.type = PLAYER_MSGTYPE_RESP_ACK;
          GlobalTime->GetTimeDouble(&resphdr.timestamp);
          resphdr.size = sizeof(player_device_resp_t);

          if(!(device = deviceTable->GetDevice(devreq->addr)))
          {
            PLAYER_WARN2("skipping subscription to unknown device %u:%u",
                         devreq->addr.interf, devreq->addr.index);
          }
          else
          {
            // copy in the driver name
            strncpy(devresp.driver_name,device->drivername,
                    sizeof(devresp.driver_name));
            devresp.driver_name_count = strlen(devresp.driver_name);
            // (un)subscribe the client to the device
            switch(devreq->access)
            {
              case PLAYER_OPEN_MODE:
                if(device->Subscribe(client->queue) != 0)
                {
                  PLAYER_WARN2("subscription failed for device %u:%u",
                               devreq->addr.interf, devreq->addr.index);
                }
                else
                  devresp.access = devreq->access;
                break;
              case PLAYER_CLOSE_MODE:
                if(device->Unsubscribe(client->queue) != 0)
                {
                  PLAYER_WARN2("unsubscription failed for device %u:%u",
                               devreq->addr.interf, devreq->addr.index);
                }
                else
                  devresp.access = devreq->access;
                break;
              default:
                PLAYER_WARN3("unknown access mode %u requested for device %u:%u",
                             devreq->access, devreq->addr.interf, 
                             devreq->addr.index);
                break;
            }
          }

          // Make up and push out the reply
          resp = new Message(resphdr, (void*)&devresp, 
                             sizeof(player_device_resp_t));
          assert(resp);
          client->queue->Push(*resp);
          delete resp;
          break;
        default:
          PLAYER_WARN1("player interface discarding message of unsupported subtype %u",
                       hdr->subtype);

          resphdr.type = PLAYER_MSGTYPE_RESP_NACK;
          GlobalTime->GetTimeDouble(&resphdr.timestamp);
          resphdr.size = 0;
          // Make up and push out the reply
          resp = new Message(resphdr, NULL, 0);
          assert(resp);
          client->queue->Push(*resp);
          delete resp;
          break;
      }
      break;
    default:
      PLAYER_WARN1("player interface discarding message of unsupported type %u",
                   hdr->type);
      resphdr.type = PLAYER_MSGTYPE_RESP_NACK;
      GlobalTime->GetTimeDouble(&resphdr.timestamp);
      resphdr.size = 0;
      // Make up and push out the reply
      resp = new Message(resphdr, NULL, 0);
      assert(resp);
      client->queue->Push(*resp);
      delete resp;
      break;
  }
  return(0);
}

