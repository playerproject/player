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
 *  class to hold info about the client reader and writer threads
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <errno.h>
#include <string.h>  // for memcpy(3)
#include <stdlib.h>  // for exit(3)
#include <sys/types.h>  // for accept(2)
#include <sys/socket.h>  // for accept(2)
#include <netinet/in.h>  // for byte-swappers
#include <fcntl.h>  // for fcntl(2)
#include <unistd.h>  // for fnctl(2)

#include <sys/time.h>  // temporary

#include "clientmanager.h"
#include "devicetable.h"
#include "device.h"


// the externed vars are declared in main.cc
#include <playertime.h>
extern PlayerTime* GlobalTime;
extern char playerversion[];
extern bool use_stage;

/* used to name incoming client connections */
int
make_dotted_ip_address(char* dest, int len, uint32_t addr)
{
  char tmp[512];
  int mask = 0xff;
  int swappedaddr;

  swappedaddr = htonl(addr);
  
  sprintf(tmp, "%u.%u.%u.%u",
                  swappedaddr>>24 & mask,
                  swappedaddr>>16 & mask,
                  swappedaddr>>8 & mask,
                  swappedaddr>>0 & mask);

  if((strlen(tmp) + 1) > (unsigned int)len)
  {
    return(-1);
  }
  else
  {
    strncpy(dest, tmp, len);
    return(0);
  }
}

ClientManager::ClientManager(struct pollfd* listen_ufds, int* ports,
                             int numfds, char* auth_key)
{
  int initial_size=8;

  curr_client_id=1;

  strncpy(client_auth_key, auth_key,sizeof(client_auth_key));
  // just in case...
  client_auth_key[sizeof(client_auth_key)-1] = '\0';

  assert(accept_ports = new int[numfds]);
  memcpy(accept_ports, ports, sizeof(int)*numfds);
  assert(accept_ufds = new struct pollfd[numfds]);
  memcpy(accept_ufds, listen_ufds, sizeof(struct pollfd)*numfds);
  num_accept_ufds = numfds;

  assert(clients = new ClientData*[initial_size]);
  memset((char*)clients,0,sizeof(ClientData*)*initial_size);
  assert(ufds = new struct pollfd[initial_size]);
  memset((char*)ufds,0,sizeof(struct pollfd)*initial_size);
  size_clients = initial_size;
  num_clients = 0;
}

// destructor
ClientManager::~ClientManager()
{
  // tear down dynamic structures here
  if(clients)
  {
    for(int i=0;i<num_clients;i++)
    {
      if(clients[i])
      {
        delete clients[i];
      }
    }
    delete[] clients;
  }
  if(ufds)
    delete[] ufds;
  if(accept_ufds)
    delete[] accept_ufds;
  if(accept_ports)
    delete[] accept_ports;
}

// add a client to our watch list
void ClientManager::AddClient(ClientData* client)
{
  int retval;
  if(!client)
    return;

  // First, add it to the array of clients

  // do we need more room?
  if(num_clients >= size_clients)
  {
    //puts("allocating more room for clients");
    ClientData** tmp_clients;

    // allocate twice as much space
    if(!(tmp_clients = new ClientData*[2*size_clients]))
    {
      fputs("ClientManager::AddClient(): new failed. Out of memory? bailing",
            stderr);
      exit(1);
    }

    // zero it
    memset((char*)tmp_clients,0,sizeof(ClientData*)*2*size_clients);

    // copy existing data
    memcpy(tmp_clients,clients,sizeof(ClientData*)*num_clients);

    // kill the old array
    delete clients;

    // swap pointers
    clients = tmp_clients;

    //puts("allocating more room for ufds");
    struct pollfd* tmp_ufds;

    // allocate twice as much space
    if(!(tmp_ufds = new struct pollfd[2*size_clients]))
    {
      fputs("ClientManager::AddClient(): new failed. Out of memory? bailing",
            stderr);
      exit(1);
    }
    
    // zero it
    memset((char*)tmp_ufds,0,sizeof(struct pollfd)*2*size_clients);

    // copy existing data
    memcpy(tmp_ufds,ufds,sizeof(struct pollfd)*num_clients);

    // kill the old array
    delete ufds;

    // swap pointers
    ufds = tmp_ufds;

    size_clients *= 2;
  }
  
  // add it to the array
  clients[num_clients]= client;

  // add it to the array
  ufds[num_clients].fd = client->socket;
  if(!(client->socket))
  {
    // it's a dummy, representing an internal subscription, so don't set the
    // events field.
    ufds[num_clients++].events = 0;
  }
  else
  {
    // we're interested in read/accept events.
    ufds[num_clients++].events = POLLIN;
  }
    
  // try to write the ident string to the client, and kill the client if
  // it fails
  unsigned char data[PLAYER_IDENT_STRLEN];
  // write back an identifier string
  if(use_stage)
    sprintf((char*)data, "%s%s (stage)", PLAYER_IDENT_STRING, playerversion);
  else
    sprintf((char*)data, "%s%s", PLAYER_IDENT_STRING, playerversion);
  memset(((char*)data)+strlen((char*)data),0,
        PLAYER_IDENT_STRLEN-strlen((char*)data));

  client->FillWriteBuffer(data,0,PLAYER_IDENT_STRLEN);
  retval= client->Write(PLAYER_IDENT_STRLEN);

  if(retval < 0)
  {
    if(errno != EAGAIN)
    {
      PLAYER_ERROR1("%s",strerror(errno));
      MarkClientForDeletion(num_clients-1);
      RemoveBlanks();
    }
  }
}
    
// call Update() on all subscribed devices
void 
ClientManager::UpdateDevices()
{
  for(CDeviceEntry* dev = deviceTable->GetFirstEntry();
      dev;
      dev = deviceTable->GetNextEntry(dev))
  {
    if(dev->devicep->subscriptions)
    {
      //printf("calling Update on %d:%d:%d\n",
             //dev->id.port,
             //dev->id.code,
             //dev->id.index);
      dev->devicep->Update();
    }
  }
}

// Update the ClientManager
int ClientManager::Update()
{
  int retval;

  if((retval = Accept()))
  {
    printf("Accept() returned %d\n", retval);
    return(retval);
  }

  if((retval = Read()))
  {
    printf("Read() returned %d\n", retval);
    return(retval);
  }

  UpdateDevices();

  if((retval = Write()))
  {
    printf("Write() returned %d\n", retval);
    return(retval);
  }

  return(0);
}

// mark a client for deletion
void
ClientManager::MarkClientForDeletion(int idx)
{
  clients[idx]->markedfordeletion = true;
}
    
// shift the clients and ufds down so that they're contiguous
void ClientManager::RemoveBlanks()
{
  int i,j;

  if(!num_clients)
    return;

  for(i=0;i<num_clients;)
  {
    if(!clients[i] || clients[i]->markedfordeletion)
    {
      if(clients[i])
      {
        delete clients[i];
        clients[i] = NULL;
      }

      for(j=i+1;j<num_clients;j++)
      {
        if(clients[j])
          break;
      }

      if(j < num_clients)
      {
        // copy it
        clients[i] = clients[j];
        clients[j] = NULL;

        ufds[i] = ufds[j];
        ufds[j].fd = -1;
        ufds[j].events = 0;
      }
    }
    i++;
  }

  // recount
  for(i=0;i<size_clients && clients[i];i++);

  num_clients = i;
}

// get the index corresponding to a ClientData pointer
int ClientManager::GetIndex(ClientData* ptr)
{
  int retval = -1;
  for(int i=0;i<num_clients;i++)
  {
    if(clients[i] == ptr)
    {
      retval = i;
      break;
    }
  }

  return(retval);
}

// Reset 'last_write' field to 0.0 in each client.  Used when playing back
// data from a logfile and a client requests the logfile be rewound.
void
ClientManager::ResetClientTimestamps(void)
{
  int i;
  for(i=0;i<num_clients;i++)
    clients[i]->last_write = 0.0;
}

/*************************************************************************
 * ClientManagerTCP
 *************************************************************************/
int
ClientManagerTCP::Accept()
{
  int num_connects;
  ClientData *clientData;
  socklen_t sender_len;

  if((num_connects = poll(accept_ufds,num_accept_ufds,0)) < 0)
  {
    if(errno == EINTR)
      return(0);

    perror("poll() failed.");
    exit(-1);
  }

  if(!num_connects)
    return(0);

  for(int i=0;i<num_accept_ufds && num_connects>0;i++)
  {
    if(accept_ufds[i].revents & POLLIN)
    {
      num_connects--;
      clientData = new ClientDataTCP(client_auth_key, accept_ports[i]);

      struct sockaddr_in cliaddr;
      sender_len = sizeof(cliaddr);
      memset(&cliaddr, 0, sizeof(cliaddr));

      /* shouldn't block here */
      if((clientData->socket = accept(accept_ufds[i].fd, 
                                      (struct sockaddr*)&cliaddr, 
                                      &sender_len)) == -1)
      {
        perror("accept(2) failed: ");
        exit(-1);
      }

      // make the socket non-blocking
      if(fcntl(clientData->socket, F_SETFL, O_NONBLOCK) == -1)
      {
        perror("fcntl() failed while making socket non-blocking. quitting.");
        exit(-1);
      }

      /* got conn */

      // now reports the port number that was connected, instead of the 
      // global player port - RTV
      char clientIp[64];
      if(make_dotted_ip_address(clientIp, 64, 
                                (uint32_t)(cliaddr.sin_addr.s_addr)))
      {
        // couldn't get the ip
        printf("** Player [port %d] client accepted on socket %d **\n",
               accept_ports[i],  clientData->socket);
      }
      else
        printf("** Player [port %d] client accepted from %s "
	     "on socket %d **\n", 
	     accept_ports[i], clientIp, clientData->socket);

      /* add it to the manager's list */
      AddClient(clientData);
    }
  }
  return(0);
}
int
ClientManagerTCP::Read()
{
  int num_to_read;

  // poll the fds that we're reading from, without timeout of 0ms, for
  // immediate return
  if((num_to_read = poll(ufds,num_clients,10)) == -1)
  {
    if(errno != EINTR)
    {
      perror("ClientManager::Read(): poll(2) failed:");
      return(-1);
    }
  }

  // call the corresponding Read() for each one that's ready
  for(int i=0;i<num_clients && num_to_read>0;i++)
  {
    // is this one ready to read?
    if(ufds[i].revents & POLLIN)
    {
      num_to_read--;
      // maybe it was just deleted?
      if(!clients[i])
        continue;
      //printf("reading from: %d 0x%x\n", i,ufds[i].events);
      if((clients[i]->Read()) == -1)
      {
        // read(2) must have errored. client is probably gone
        MarkClientForDeletion(i);
      }
    }
    else if(ufds[i].revents)
    {
      if(!(ufds[i].revents & POLLHUP))
        printf("ClientManager::Read() got strange revent 0x%x for "
               "client %d; killing it\n", ufds[i].revents,i);
      MarkClientForDeletion(i);
    }
  }

  RemoveBlanks();
  return(0);
}

int
ClientManagerTCP::Write()
{
  struct timeval curr;
  
  if(GlobalTime->GetTime(&curr) == -1)
    fputs("ClientManager::Write(): GetTime() failed!!!!\n", stderr);

  for(int i=0;i<num_clients;i++)
  {
    // if this is a dummy, skip it.
    if(!(clients[i]->socket))
      continue;

    // if we're waiting for an authorization on this client, then skip it
    if(clients[i]->auth_pending)
      continue;

    // if this client has data waiting to be sent, try to send it
    if(clients[i]->leftover_size)
    {
      if(clients[i]->Write(clients[i]->leftover_size) < 0)
        MarkClientForDeletion(i);

      // even if the Write() succeeded, skip this client for this round
      continue;
    }

    // look for pending replies intended for this client.  we only need to 
    // look in the devices to which this client is subscribed, thus we
    // iterate through the client's own subscription list.
    for(CDeviceSubscription* thisub = clients[i]->requested;
        thisub;
        thisub = thisub->next)
    {
      unsigned short type;
      int replysize;
      struct timeval ts;
      player_device_id_t id;
      memset(&id,0,sizeof(id));

      // if this client has built up leftover outgoing data as a result of a
      // previous reply not getting sent, don't add any more replies.
      if(clients[i]->leftover_size)
        break;

      // is this a valid device
      if(thisub->devicep)
      {
        // does this device have a reply ready for this client?
        if((replysize = thisub->devicep->GetReply(&id, clients[i], &type, &ts,
                  clients[i]->replybuffer+sizeof(player_msghdr_t), 
                  PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))) >= 0)
        {
          // build up and send the reply
          player_msghdr_t reply_hdr;

          reply_hdr.stx = htons(PLAYER_STXX);
          reply_hdr.type = htons(type);

          // if the device code is 0 (which is invalid), then the device
          // didn't specify it, so assume that the identifying info in the
          // subscription entry is correct
          if(!id.code)
          {
            reply_hdr.device = htons(thisub->id.code);
            reply_hdr.device_index = htons(thisub->id.index);
          }
          else
          {
            reply_hdr.device = htons(id.code);
            reply_hdr.device_index = htons(id.index);
          }
          reply_hdr.reserved = (uint32_t)0;
          reply_hdr.size = htonl(replysize);

          if(GlobalTime->GetTime(&curr) == -1)
            fputs("ClientWriterThread(): GetTime() failed!!!!\n", stderr);
          reply_hdr.time_sec = htonl(curr.tv_sec);
          reply_hdr.time_usec = htonl(curr.tv_usec);
          reply_hdr.timestamp_sec = htonl(ts.tv_sec);
          reply_hdr.timestamp_usec = htonl(ts.tv_usec);

          memcpy(clients[i]->replybuffer,&reply_hdr,
                 sizeof(player_msghdr_t));

          clients[i]->FillWriteBuffer(clients[i]->replybuffer,
                                      0,
                                      replysize+sizeof(player_msghdr_t));

          if(clients[i]->Write(replysize+sizeof(player_msghdr_t)) < 0)
          {
            // dump this client
            MarkClientForDeletion(i);
            // break out of device loop
            break;
          }
        }
      }
    }

    // if this client has built up leftover outgoing data as a result of a
    // reply not getting sent, don't add any data
    if(clients[i]->leftover_size)
      continue;

    // rtv - had to fix a dumb rounding error here. This code is
    // producing intervals like 0.09999-recurring seconds instead of
    // 0.1 second, so updates were being skipped. I added a
    // microsecond (*) when testing the elapsed interval. The bug was
    // probably not a problem when using the real-time clock, but
    // shows up when working with a simulator where time comes in
    // discrete chunks. This was a beast to figure out since printf
    // was rounding up the numbers to print them out!
    
    double curr_seconds = curr.tv_sec+(curr.tv_usec/1000000.0);
    
    // is it time to write?
    if((((clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
         (clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW)) &&
        (((curr_seconds + 0.000001) - clients[i]->last_write) >=  // * 
         (1.0/clients[i]->frequency))) ||
       (((clients[i]->mode == PLAYER_DATAMODE_PULL_ALL) ||
         (clients[i]->mode == PLAYER_DATAMODE_PULL_NEW)) &&
        (clients[i]->datarequested)))
    {
      size_t msglen = clients[i]->BuildMsg();
      if(clients[i]->Write(msglen) == -1)
        MarkClientForDeletion(i);
      else
      {
        if((clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
           (clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW))
          clients[i]->last_write = curr_seconds;
        else
          clients[i]->datarequested = false;
      }
    }
  }


  // remove any clients that we marked
  RemoveBlanks();

  return(0);
}

/*************************************************************************
 * ClientManagerUDP
 *************************************************************************/
int ClientManagerUDP::Accept()
{
  // UDP doesn't have an accept step
  return(0);
}

int
ClientManagerUDP::Read()
{
  int num_to_read;
  int numread;
  unsigned char buf[PLAYER_MAX_MESSAGE_SIZE];
  char senderaddr[256];
  struct sockaddr_in sender;
  socklen_t senderlen = sizeof(sender);
  ClientData *clientData;
  player_msghdr_t hdr;

  // poll the fds that we're listening on, with smallest possible timeout,
  // just so that we yield the processor.
  if((num_to_read = poll(accept_ufds,num_accept_ufds,1)) == -1)
  {
    if(errno != EINTR)
    {
      PLAYER_ERROR1("%s", strerror(errno));
      return(-1);
    }
  }

  // call the corresponding Read() for each one that's ready
  for(int i=0;i<num_accept_ufds && num_to_read>0;i++)
  {
    clientData=NULL;
    // is this one ready to read?
    if(accept_ufds[i].revents & POLLIN)
    {
      num_to_read--;

      // Read to get the sender's address, but leave the message on the queue, 
      // so that it can be read by the appropriate client object.
      if((numread = recvfrom(accept_ufds[i].fd,(void*)&hdr,sizeof(hdr),
                             MSG_PEEK,(struct sockaddr*)&sender,
                             &senderlen)) < 0)
      {
        PLAYER_ERROR1("%s",strerror(errno));
        return(-1);
      }

      make_dotted_ip_address(senderaddr, sizeof(senderaddr),
                             sender.sin_addr.s_addr);

      // if the client ID (the first 2 bytes of reserved) is 0, then this 
      // must be a new client
      if((ntohs(hdr.stx) == PLAYER_STXX) &&
         (ntohs(hdr.reserved >> 16) == 0) &&
         (ntohs(hdr.type) == PLAYER_MSGTYPE_REQ) &&
         (ntohs(hdr.device) == PLAYER_PLAYER_CODE) &&
         (ntohs(hdr.device_index) == 0) &&
         (ntohl(hdr.size) == 0))
      {
        // no existing client object; create a new one
        clientData = new ClientDataUDP(client_auth_key, accept_ports[i]);
        memcpy(&clientData->clientaddr,&sender,senderlen);
        clientData->clientaddr_len = senderlen;
        clientData->socket = accept_ufds[i].fd;
        clientData->client_id = curr_client_id++;

        /* got new conn */

        char clientIp[64];
        if(make_dotted_ip_address(clientIp, sizeof(clientIp), 
                                  sender.sin_addr.s_addr))
        {
          // couldn't get the ip
          printf("** Player [port %d] client accepted on socket %d **\n",
                 accept_ports[i],  clientData->socket);
        }
        else
          printf("** Player [port %d] client accepted from %s "
                 "on socket %d **\n", 
                 accept_ports[i], clientIp, clientData->socket);

        // add it to the manager's list
        AddClient(clientData);

        // send a zero-length ACK so that the client knows his own ID
        struct timeval curr;
        GlobalTime->GetTime(&curr);
        hdr.type = htons(PLAYER_MSGTYPE_RESP_ACK);
        hdr.time_sec = hdr.timestamp_sec = htonl(curr.tv_sec);
        hdr.time_usec = hdr.timestamp_usec = htonl(curr.tv_usec);
        hdr.reserved = htons(clientData->client_id) << 16;

        clientData->FillWriteBuffer((unsigned char*)&hdr,0,sizeof(hdr));
        if(clientData->Write(sizeof(hdr)) < 0)
        {
          PLAYER_ERROR1("%s", strerror(errno));
          return(-1);
        }
        // consume the message
        if((numread = recvfrom(accept_ufds[i].fd,&buf,sizeof(buf),
                               0, NULL, NULL)) < 0)
        {
          PLAYER_ERROR1("%s",strerror(errno));
          return(-1);
        }
        continue;
      }

      // is there an object for this client yet?
      for(int j=0; j<num_clients && clients[j]; j++)
      {
        if(clients[j]->client_id == ntohs(hdr.reserved >> 16))
        {
          clientData=clients[j];
          break;
        }
      }

      if(!clientData)
      {
        PLAYER_WARN("client sent message with invalid client ID");
        // consume the message
        if((numread = recvfrom(accept_ufds[i].fd,&buf,sizeof(buf),
                               0, NULL, NULL)) < 0)
        {
          PLAYER_ERROR1("%s",strerror(errno));
          return(-1);
        }
        continue;
      }
      
      if((clientData->Read()) == -1)
      {
        //MarkClientForDeletion(i);
        clientData->markedfordeletion = true;
      }
    }
    else if(accept_ufds[i].revents)
    {
      if(!(accept_ufds[i].revents & POLLHUP))
        printf("ClientManager::Read() got strange revent 0x%x for "
               "port %d\n", accept_ufds[i].revents,accept_ports[i]);
    }
  }

  RemoveBlanks();
  return(0);
}

int
ClientManagerUDP::Write()
{
  struct timeval curr;
  player_msghdr_t* hdrp;
  size_t totalmsgsize, msgsize, p;
  
  if(GlobalTime->GetTime(&curr) == -1)
    fputs("ClientManager::Write(): GetTime() failed!!!!\n", stderr);

  for(int i=0;i<num_clients;i++)
  {
    // if we're waiting for an authorization on this client, then skip it
    if(clients[i]->auth_pending)
      continue;

    // look for pending replies intended for this client.  we only need to 
    // look in the devices to which this client is subscribed, thus we
    // iterate through the client's own subscription list.
    for(CDeviceSubscription* thisub = clients[i]->requested;
        thisub;
        thisub = thisub->next)
    {
      unsigned short type;
      int replysize;
      struct timeval ts;
      player_device_id_t id;
      memset(&id,0,sizeof(id));

      // is this a valid device
      if(thisub->devicep)
      {
        // does this device have a reply ready for this client?
        if((replysize = thisub->devicep->GetReply(&id, clients[i], &type, &ts,
                  clients[i]->replybuffer+sizeof(player_msghdr_t), 
                  PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t))) >= 0)
        {
          // build up and send the reply
          player_msghdr_t reply_hdr;

          reply_hdr.stx = htons(PLAYER_STXX);
          reply_hdr.type = htons(type);

          // if the device code is 0 (which is invalid), then the device
          // didn't specify it, so assume that the identifying info in the
          // subscription entry is correct
          if(!id.code)
          {
            reply_hdr.device = htons(thisub->id.code);
            reply_hdr.device_index = htons(thisub->id.index);
          }
          else
          {
            reply_hdr.device = htons(id.code);
            reply_hdr.device_index = htons(id.index);
          }
          reply_hdr.reserved = (uint32_t)0;
          reply_hdr.size = htonl(replysize);

          if(GlobalTime->GetTime(&curr) == -1)
            fputs("ClientWriterThread(): GetTime() failed!!!!\n", stderr);
          reply_hdr.time_sec = htonl(curr.tv_sec);
          reply_hdr.time_usec = htonl(curr.tv_usec);
          reply_hdr.timestamp_sec = htonl(ts.tv_sec);
          reply_hdr.timestamp_usec = htonl(ts.tv_usec);

          memcpy(clients[i]->replybuffer,&reply_hdr,
                 sizeof(player_msghdr_t));

          clients[i]->FillWriteBuffer(clients[i]->replybuffer,
                                      0,
                                      replysize+sizeof(player_msghdr_t));

          if(clients[i]->Write(replysize+sizeof(player_msghdr_t)) < 0)
          {
            // dump this client
            MarkClientForDeletion(i);
            // break out of device loop
            break;
          }
        }
      }
    }

    // if this client has built up leftover outgoing data as a result of a
    // reply not getting sent, don't add any data
    if(clients[i]->leftover_size)
      continue;
    
    // is it time to write?
    if((((clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
         (clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW)) &&
        (((curr.tv_sec+(curr.tv_usec/1000000.0))- clients[i]->last_write) >= 
         (1.0/clients[i]->frequency))) ||
       (((clients[i]->mode == PLAYER_DATAMODE_PULL_ALL) ||
         (clients[i]->mode == PLAYER_DATAMODE_PULL_NEW)) &&
        (clients[i]->datarequested)))
    {
      totalmsgsize = clients[i]->BuildMsg();

      // now, loop through that buffer and send the messages as individual
      // datagrams
      for(p=0; p < totalmsgsize; p += msgsize)
      {
        hdrp = (player_msghdr_t*)(clients[i]->totalwritebuffer);
        msgsize = sizeof(player_msghdr_t) + ntohl(hdrp->size);

        if(clients[i]->Write(msgsize) == -1)
        {
          MarkClientForDeletion(i);
          break;
        }

        // move the next msg up
        memmove(clients[i]->totalwritebuffer,
                clients[i]->totalwritebuffer+msgsize,
                totalmsgsize-msgsize);
      }


      // did we write them all succesfully?
      if(p == totalmsgsize)
      {
        if((clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
           (clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW))
          clients[i]->last_write = curr.tv_sec + curr.tv_usec / 1000000.0;
        else
          clients[i]->datarequested = false;
      }
    }
  }

  // remove any clients that we marked
  RemoveBlanks();

  return(0);
}
