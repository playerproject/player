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
#include <string.h>  // for memcpy(3)
#include <stdlib.h>  // for exit(3)
#include <signal.h>  // for sigblock(2)
#include <sys/time.h>  // for sigblock(2)
#include <unistd.h>  // for usleep(2)

void* ClientReaderThread(void* arg);
void* ClientWriterThread(void* arg);

ClientManager::ClientManager()
{
  // initialize both arrays to this size; we'll double it each time
  // we need more space later
  int initial_size = 8;

  if(!(clients = new CClientData*[initial_size]))
  {
    fputs("ClientManager::ClientManager(): new failed. "
          "Out of memory? bailing\n", stderr);
    exit(1);
  }
  bzero(clients,sizeof(CClientData*)*initial_size);
  size_clients = initial_size;
  num_clients = 0;

  if(!(ufds = new struct pollfd[initial_size]))
  {
    fputs("PlayerMultiClient::PlayerMultiClient(): new failed. "
          "Out of memory? bailing \n", stderr);
    exit(1);
  }
  bzero(ufds,sizeof(struct pollfd)*initial_size);
  size_ufds = initial_size;
  num_ufds = 0;

  pthread_mutex_init(&mutex,NULL);

  // start the reader thread
  if(pthread_create(&readthread, NULL, ClientReaderThread, this))
  {
    perror("pthread_create(3) failed; bailing");
    exit(-1);
  }
  
  // start the writer thread
  if(pthread_create(&writethread, NULL, ClientWriterThread, this))
  {
    perror("pthread_create(3) failed; bailing");
    exit(-1);
  }
}

// destructor
ClientManager::~ClientManager()
{
  pthread_cancel(readthread);
  
  // tear down dynamic structures here
  if(clients)
    delete clients;
  if(ufds)
    delete ufds;

  pthread_mutex_destroy(&mutex);
}

// add a client to our watch list
void ClientManager::AddClient(CClientData* client)
{
  if(!client)
    return;

  pthread_mutex_lock(&mutex);

  // First, add it to the array of clients

  // do we need more room?
  if(num_clients >= size_clients)
  {
    puts("allocating more room for clients");
    CClientData** tmp_clients;

    // allocate twice as much space
    if(!(tmp_clients = new CClientData*[2*size_clients]))
    {
      fputs("ClientManager::AddClient(): new failed. Out of memory? bailing",
            stderr);
      exit(1);
    }

    // zero it
    bzero(tmp_clients,sizeof(CClientData*)*2*size_clients);

    // copy existing data
    memcpy(tmp_clients,clients,sizeof(CClientData*)*num_clients);

    // kill the old array
    delete clients;

    // swap pointers
    clients = tmp_clients;
    size_clients *= 2;
  }

  // add it to the array
  clients[num_clients++] = client;

  // Now, add it to our poll watchlist

  // do we need more room?
  if(num_ufds >= size_ufds)
  {
    puts("allocating more room for ufds");
    struct pollfd* tmp_ufds;

    // allocate twice as much space
    if(!(tmp_ufds = new struct pollfd[2*size_ufds]))
    {
      fputs("ClientManager::AddClient(): new failed. Out of memory? bailing",
            stderr);
      exit(1);
    }
    
    // zero it
    bzero(tmp_ufds,sizeof(struct pollfd)*2*size_ufds);

    // copy existing data
    memcpy(tmp_ufds,ufds,sizeof(struct pollfd)*num_ufds);

    // kill the old array
    delete ufds;

    // swap pointers
    ufds = tmp_ufds;
    size_ufds *= 2;
  }

  // add it to the array
  ufds[num_ufds].fd = client->socket;
  ufds[num_ufds++].events = POLLIN;
    
  if(client->WriteIdentString() == -1)
    RemoveClient(num_clients-1);
  pthread_mutex_unlock(&mutex);
}
    
// remove a client
void ClientManager::RemoveClient(int idx)
{
  //puts("calling RemoveClient()");

  if(clients[idx])
  {
    delete clients[idx];
    clients[idx] = (CClientData*)NULL;
  }
  
  // now shift the remaining ones down so they're contiguous.
  for(int i=idx,j=idx+1;j<num_ufds;i++,j++)
    clients[i] = clients[j];
  num_clients--;

  if(ufds[idx].events)
  {
    ufds[idx].fd = -1;
    ufds[idx].events = 0;
  }

  // now shift the remaining ones down so they're contiguous.
  for(int i=idx,j=idx+1;j<num_ufds;i++,j++)
    ufds[i] = ufds[j];
  num_ufds--;
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

int ClientManager::Read()
{
  int num_to_read,retval;
  
  pthread_mutex_lock(&mutex);
  // let's try this poll(2) thing (with a 100ms timeout, because we need to 
  // notice new clients added to our watch list)

  //printf("calling poll with %d\n", num_ufds);
  //if(num_ufds>=1)
    //printf("ufds[0].fd : %d  ufds[0].events: 0x%x\n", 
           //ufds[0].fd,ufds[0].events);
  if((num_to_read = poll(ufds,num_ufds,100)) == -1)
  {
    perror("ClientManager::Read(): poll(2) failed:");
    pthread_mutex_unlock(&mutex);
    return(-1);
  }

  //if(num_to_read)
    //printf("%d to read\n", num_to_read);
  //printf("EVENTS: %d\n", num_to_read);
  // call the corresponding Read() for each one that's ready
  for(int i=0;i<num_ufds && num_to_read;i++)
  {
    // is this one ready to read?
    if(ufds[i].revents & POLLIN)
    {
      num_to_read--;
      //printf("reading from: %d 0x%x\n", i,ufds[i].events);
      if((retval = clients[i]->Read()) == -1)
      {
        // read(2) must have errored. client is probably gone
        RemoveClient(i);
      }
    }
    else if(ufds[i].revents)
    {
      printf("ClientManager::Read() got strange revent 0x%x for "
             "client %d\n", ufds[i].revents,i);
    }
  }

  pthread_mutex_unlock(&mutex);
  return(0);
}

void*
ClientReaderThread(void* arg)
{
  ClientManager* cr = (ClientManager*)arg;

  pthread_detach(pthread_self());

#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif

  for(;;)
    cr->Read();
}

void*
ClientWriterThread(void* arg)
{
  ClientManager* cr = (ClientManager*)arg;
  struct timeval curr;

  pthread_detach(pthread_self());

#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif

  
  for(;;)
  {
    // sleep a bit, then check to see if anybody needs taking care of
    usleep(10000);

    gettimeofday(&curr,NULL);

    pthread_mutex_lock(&(cr->mutex));
    for(int i=0;i<cr->num_clients;i++)
    {
      if(cr->clients[i]->auth_pending)
        continue;

      // is it time to write?
      if(cr->clients[i]->mode == CONTINUOUS || cr->clients[i]->mode == UPDATE)
      {
        if(((curr.tv_sec+(curr.tv_usec/1000000.0))-
            cr->clients[i]->last_write) >= (1.0/cr->clients[i]->frequency))
        {
          cr->clients[i]->Write();
        }
      }

    
      /*
      if(clientData->mode == CONTINUOUS || clientData->mode == UPDATE )
      {
        // account for 10ms delay
        realsleep = (unsigned long)(1000000.0 / clientData->frequency)-10000;
        realsleep = max(realsleep,0);
        //usleep((unsigned long)(1000000.0 / clientData->frequency));
        usleep(realsleep);
      }
      else if(clientData->mode == REQUESTREPLY)
      {
        pthread_mutex_lock(&(clientData->datarequested));
      }
      */
    }
    pthread_mutex_unlock(&(cr->mutex));
  }
}

