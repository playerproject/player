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

#include "clientmanager.h"
#include "device.h"

#if HAVE_STRINGS_H
  #include <strings.h>
#endif

#include <errno.h>
#include <string.h>  // for memcpy(3)
#include <stdlib.h>  // for exit(3)
#include <netinet/in.h>  // for byte-swappers
#include <sys/types.h>  // for accept(2)
#include <sys/socket.h>  // for accept(2)
#include <fcntl.h>  // for fcntl(2)
#include <unistd.h>  // for fnctl(2)

#include <sys/time.h>  // temporary

#include <playertime.h>
extern PlayerTime* GlobalTime;

/* used to name incoming client connections */
int
make_dotted_ip_address(char* dest, int len, uint32_t addr)
{
  char tmp[512];
  int mask = 0xff;

  sprintf(tmp, "%u.%u.%u.%u",
                  addr>>0 & mask,
                  addr>>8 & mask,
                  addr>>16 & mask,
                  addr>>24 & mask);

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
  // allocate space for accept_ufds
  if(!(accept_ufds = new struct pollfd[numfds]) || 
     !(accept_ports = new int[numfds]))
  {
    fputs("ClientManager::ClientManager(): new failed. "
          "Out of memory? bailing\n", stderr);
    exit(1);
  }

  // store the ufds and ports for accepting new connections
  memcpy(accept_ufds, listen_ufds, sizeof(struct pollfd)*numfds);
  memcpy(accept_ports, ports, sizeof(int)*numfds);
  num_accept_ufds = numfds;

  strncpy(client_auth_key, auth_key,sizeof(client_auth_key));
  // just in case...
  client_auth_key[sizeof(client_auth_key)-1] = '\0';

  // initialize both arrays to this size; we'll double it each time
  // we need more space later
  int initial_size = 8;

  if(!(clients = new CClientData*[initial_size]))
  {
    fputs("ClientManager::ClientManager(): new failed. "
          "Out of memory? bailing\n", stderr);
    exit(1);
  }
  bzero((char*)clients,sizeof(CClientData*)*initial_size);
  size_clients = initial_size;
  num_clients = 0;

  if(!(ufds = new struct pollfd[initial_size]))
  {
    fputs("ClientManager::ClientManager(): new failed. "
          "Out of memory? bailing \n", stderr);
    exit(1);
  }
  bzero((char*)ufds,sizeof(struct pollfd)*initial_size);
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
    delete clients;
  }
  if(ufds)
    delete ufds;
  if(accept_ufds)
    delete accept_ufds;
}

// add a client to our watch list
void ClientManager::AddClient(CClientData* client)
{
  if(!client)
    return;

  // First, add it to the array of clients

  // do we need more room?
  if(num_clients >= size_clients)
  {
    //puts("allocating more room for clients");
    CClientData** tmp_clients;

    // allocate twice as much space
    if(!(tmp_clients = new CClientData*[2*size_clients]))
    {
      fputs("ClientManager::AddClient(): new failed. Out of memory? bailing",
            stderr);
      exit(1);
    }

    // zero it
    bzero((char*)tmp_clients,sizeof(CClientData*)*2*size_clients);

    // copy existing data
    memcpy(tmp_clients,clients,sizeof(CClientData*)*num_clients);

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
    bzero((char*)tmp_ufds,sizeof(struct pollfd)*2*size_clients);

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
  ufds[num_clients++].events = POLLIN;
    
  // try to write the ident string to the client, and kill the client if
  // it fails
  if(client->WriteIdentString() == -1)
  {
    MarkClientForDeletion(num_clients-1);
    RemoveBlanks();
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
  for(i=0;clients[i];i++);

  num_clients = i;
}

// get the index corresponding to a CClientData pointer
int ClientManager::GetIndex(CClientData* ptr)
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

// accept new connections
int ClientManager::Accept()
{
  int num_connects;
  CClientData *clientData;
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
      clientData = new CClientData(client_auth_key,accept_ports[i]);

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

int ClientManager::Read()
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

int ClientManager::Write()
{
  struct timeval curr;
  
  if(GlobalTime->GetTime(&curr) == -1)
    fputs("ClientManager::Write(): GetTime() failed!!!!\n", stderr);

  for(int i=0;i<num_clients;i++)
  {
    // if we're waiting for an authorization on this client, then skip it
    if(clients[i]->auth_pending)
      continue;

    // look for pending replies intended for this client.  we only need to 
    // look in the devices to which this client is subscribed, thus we
    // iterate through the client's own subscription list (this structure
    // is already locked above by the 'access' mutex).
    for(CDeviceSubscription* thisub = clients[i]->requested;
        thisub;
        thisub = thisub->next)
    {
      unsigned short type;
      int replysize;
      struct timeval ts;
      player_device_id_t id;

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

          if(write(clients[i]->socket, clients[i]->replybuffer, 
                   replysize+sizeof(player_msghdr_t)) < 0) 
          {
            if(errno != EAGAIN)
            {
              perror("ClientWriterThread: write()");
              // dump this client
              MarkClientForDeletion(i);
              // break out of device loop
              break;
            }
          }
        }
      }
    }

    // is it time to write?
    if((clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
       (clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW))
    {
      /*
      if(((curr.tv_sec+(curr.tv_usec/1000000.0))-
          clients[i]->last_write) + 0.005 >= 
         (1.0/clients[i]->frequency))
         */
      if(((curr.tv_sec+(curr.tv_usec/1000000.0))-
          clients[i]->last_write) >= 
         (1.0/clients[i]->frequency))
      {
        if(clients[i]->Write() == -1)
        {
          // write must have errored. dump it
          MarkClientForDeletion(i);
        }
        else
          clients[i]->last_write = curr.tv_sec + curr.tv_usec / 1000000.0;
      }
    }
    else if(((clients[i]->mode == PLAYER_DATAMODE_PULL_ALL) ||
             (clients[i]->mode == PLAYER_DATAMODE_PULL_NEW))&&
            (clients[i]->datarequested))
    {
      clients[i]->datarequested = false;
      if(clients[i]->Write() == -1)
      {
        // write must have errored. dump it
        MarkClientForDeletion(i);
      }
    }
  }

  // remove any clients that we marked
  RemoveBlanks();

  return(0);
}
