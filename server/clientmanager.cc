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
#include "error.h"
#include "message.h"

#include "replace.h"  /* for poll(2) */

// the externed vars are declared in main.cc
#include "playertime.h"
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

  pthread_mutex_init(&this->condMutex,NULL);
  pthread_cond_init(&this->cond,NULL);
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
void
ClientManager::AddClient(ClientData* client)
{
  if(!client)
    return;

  // First, add it to the array of clients

  // do we need more room?
  if(this->num_clients >= this->size_clients)
  {
    this->size_clients *= 2;

    //puts("allocating more room for clients");
    ClientData** tmp_clients;

    // allocate twice as much space
    assert(tmp_clients = new ClientData*[this->size_clients]);

    // zero it
    memset((char*)tmp_clients,0,sizeof(ClientData*)*this->size_clients);

    // copy existing data
    memcpy(tmp_clients,this->clients,sizeof(ClientData*)*this->num_clients);

    // kill the old array
    delete[] clients;

    // swap pointers
    clients = tmp_clients;

    struct pollfd* tmp_ufds;

    // allocate twice as much space
    assert(tmp_ufds = new struct pollfd[this->size_clients]);
    
    // zero it
    memset((char*)tmp_ufds,0,sizeof(struct pollfd)*this->size_clients);

    // copy existing data
    memcpy(tmp_ufds,this->ufds,sizeof(struct pollfd)*this->num_clients);

    // kill the old array
    delete[] this->ufds;

    // swap pointers
    this->ufds = tmp_ufds;
  }
  
  // add it to the array
  this->clients[this->num_clients]= client;

  // add it to the array
  this->ufds[this->num_clients].fd = client->socket;
  if(client->socket < 0)
  {
    // it's a dummy, representing an internal subscription, so don't set the
    // events field.
    this->ufds[num_clients++].events = 0;
  }
  else
  {
    // we're interested in read/accept events.
    this->ufds[num_clients++].events = POLLIN;
  }
    
  // If the client's socket fd is < 0, then there's not really a client; this
  // device is alwayson.  So don't send the ident string.
  if(client->socket >= 0)
  {
    // Try to write the ident string to the client, and kill the client if
    // it fails.  
    unsigned char data[PLAYER_IDENT_STRLEN];
    player_msghdr_t hdr = {0};
    hdr.stx = PLAYER_STXX;
    hdr.device = PLAYER_PLAYER_CODE;
    hdr.device_index = 0;
    hdr.type = PLAYER_MSGTYPE_REQ;
    hdr.subtype = PLAYER_PLAYER_IDENT;
    hdr.size = PLAYER_IDENT_STRLEN;
    
    // write back an identifier string
    if(use_stage)
      sprintf((char*)data, "%s%s (stage)", PLAYER_IDENT_STRING, playerversion);
    else
      sprintf((char*)data, "%s%s", PLAYER_IDENT_STRING, playerversion);
    memset(((char*)data)+strlen((char*)data),0,
	   PLAYER_IDENT_STRLEN-strlen((char*)data));

    // Make a message with the reply string
    Message ident_msg(hdr, data, PLAYER_IDENT_STRLEN, client);
    // Push it onto the new client's queue
    puts("pushing");
    client->OutQueue.Push(ident_msg);
    puts("done pushing");

    // Write to the client.
    if(client->Write() < 0)
    {
      if(errno != EAGAIN)
      {
        PLAYER_ERROR1("%s",strerror(errno));
        MarkClientForDeletion(num_clients-1);
        RemoveBlanks();
      }
    }
  }
}

// remove a client from our watch list
// used when we dont want to delete the object (ie it has been allocated elsewhere)
void ClientManager::RemoveClient(ClientData* client)
{
	int ret = GetIndex(client);
	if (ret < 0)
		return;
	clients[ret] = NULL;
}

// call Update() on all subscribed devices
void 
ClientManager::UpdateDevices()
{
  for(Device* dev = deviceTable->GetFirstDevice();
      dev;
      dev = deviceTable->GetNextDevice(dev))
  {
    if(dev->driver->subscriptions)
    {
      /*
      printf("calling Update on %d:%d:%d\n",
             dev->id.port,
             dev->id.code,
             dev->id.index);
             */
      dev->driver->Update();
    }
  }
}

extern double totalwritetime;
extern int totalwritenum;

// Update the ClientManager
int 
ClientManager::Update()
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

  this->Wait();

  return(0);
}

// mark a client for deletion
void
ClientManager::MarkClientForDeletion(int idx)
{
  clients[idx]->markedfordeletion = true;
}
    
// shift the clients and ufds down so that they're contiguous
void 
ClientManager::RemoveBlanks()
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
int 
ClientManager::GetIndex(ClientData* ptr)
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

// Waits on the manager's condition variable
void 
ClientManager::Wait(void)
{
  // need to push this cleanup function, cause if a thread is cancelled while
  // in pthread_cond_wait(), it will immediately relock the mutex.  thus we
  // need to unlock ourselves before exiting.
  pthread_cleanup_push((void(*)(void*))pthread_mutex_unlock,
                       (void*)&this->condMutex);
  pthread_mutex_lock(&this->condMutex);
  pthread_cond_wait(&this->cond,&this->condMutex);
  pthread_mutex_unlock(&this->condMutex);
  pthread_cleanup_pop(1);
}

// Signal that new data is available (calls pthread_cond_broadcast()
// on the clientmanager's condition variable, which will wake it up.
void 
ClientManager::DataAvailable(void)
{
  pthread_mutex_lock(&this->condMutex);
  pthread_cond_broadcast(&this->cond);
  pthread_mutex_unlock(&this->condMutex);
}

void
ClientManager::PutMsg(uint8_t type, 
                      uint8_t subtype, 
                      uint16_t device, 
                      uint16_t device_index, 
                      struct timeval* timestamp,
                      uint32_t size, 
                      unsigned char * data, 
                      ClientData * client)
{
  // if client != Null we just send to that client
  if(client)
  {
    client->PutMsg(type, subtype, device, 
                   device_index, timestamp, size, data);
  }
  else if(this->clients)
  {
    for(int i=0; i < this->num_clients; i++)
    {
      for(DeviceSubscription* thissub=this->clients[i]->requested;
          thissub;
          thissub=thissub->next)
      {
        if(thissub->id.code == device && thissub->id.index == device_index)
        {
          this->clients[i]->PutMsg(type, subtype, device, device_index, 
                                   timestamp, size, data);
          break;
        }
      }
    }
  }
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

  // poll on the fds that we're listening on
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

  // poll on the fds that we're reading from
  if((num_to_read = poll(ufds,num_clients,0)) == -1)
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
      //puts("done reading");
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
  ClientDataTCP* cl;

  if(GlobalTime->GetTime(&curr) == -1)
    fputs("ClientManager::Write(): GetTime() failed!!!!\n", stderr);

  for(int i=0; i < this->num_clients; i++)
  {
    // FIXME: is it ok to comment out this check?
    // if this is a dummy, skip it.
    //if(clients[i]->socket < 0)
    //	continue;

    cl = (ClientDataTCP*)clients[i];

    // if we're waiting for an authorization on this client, then skip it
    if(cl->auth_pending)
      continue;

    if(cl->leftover_size)
    {
      cl->leftover_size = cl->Write();
      if(cl->leftover_size < 0)
	MarkClientForDeletion(i);
      // dont add any more data if we still have leftover from previous
      else if(cl->leftover_size > 0)
	continue;
    }

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
    bool time_to_write = 
	    ((curr_seconds + 0.000001) - cl->last_write) >=  // *
   	    (1.0/cl->frequency);

    if((cl->mode == PLAYER_DATAMODE_PUSH_ASYNC) ||
       (((cl->mode == PLAYER_DATAMODE_PUSH_ALL) || 
	 (cl->mode == PLAYER_DATAMODE_PUSH_NEW)) &&
	time_to_write) ||
       (((cl->mode == PLAYER_DATAMODE_PULL_ALL) ||
	 (cl->mode == PLAYER_DATAMODE_PULL_NEW)) &&
	(cl->datarequested)))
    {
      if (time_to_write || cl->datarequested)
      {
	// Put sync message into clients outgoing queue
	cl->PutMsg(PLAYER_MSGTYPE_SYNCH, 0, PLAYER_PLAYER_CODE, 
                   0,&curr,0,NULL);
      }

      if(cl->Write() < 0)
	MarkClientForDeletion(i);
      else
      {
	if((cl->mode == PLAYER_DATAMODE_PUSH_ALL) || 
	   (cl->mode == PLAYER_DATAMODE_PUSH_NEW))
	  cl->last_write = curr_seconds;
	else
	  cl->datarequested = false;
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
         (ntohs(hdr.conid) == 0) &&
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
        hdr.conid = htons(clientData->client_id);

        Message New(hdr,NULL,0);
        clientData->OutQueue.Push(New);
        if(clientData->Write() < 0)
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
        if(clients[j]->client_id == ntohs(hdr.conid))
        {
          clientData = clients[j];
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
        clientData->markedfordeletion = true;
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

// need to fix this once TCP version is all go
int ClientManagerUDP::Write()
{
  return 0;
}

#if 0

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
    for(DeviceSubscription* thisub = clients[i]->requested;
        thisub;
        thisub = thisub->next)
    {
      unsigned short type;
      int replysize;
      struct timeval ts;

      // is this a valid device
      if(thisub->driver)
      {
        // does this device have a reply ready for this client?
        if((replysize = thisub->driver->GetReply(thisub->id, clients[i], &type, 
                  clients[i]->replybuffer+sizeof(player_msghdr_t), 
                  PLAYER_MAX_MESSAGE_SIZE-sizeof(player_msghdr_t),&ts)) >= 0)
        {
          // build up and send the reply
          player_msghdr_t reply_hdr;

          reply_hdr.stx = htons(PLAYER_STXX);
          reply_hdr.type = htons(type);

          // TODO: check that this routing info is correct
          reply_hdr.device = htons(thisub->id.code);
          reply_hdr.device_index = htons(thisub->id.index);
          
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
    bool time_to_write = 
            ((curr_seconds + 0.000001) - clients[i]->last_write) >=  // *
            (1.0/clients[i]->frequency);

    if((clients[i]->mode == PLAYER_DATAMODE_PUSH_ASYNC) ||
       (((clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
         (clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW)) &&
        time_to_write) ||
       (((clients[i]->mode == PLAYER_DATAMODE_PULL_ALL) ||
         (clients[i]->mode == PLAYER_DATAMODE_PULL_NEW)) &&
        (clients[i]->datarequested)))
    {
      totalmsgsize = clients[i]->BuildMsg(time_to_write || 
                                          clients[i]->datarequested);

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
#endif
