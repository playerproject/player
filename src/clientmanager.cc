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
#include <errno.h>
#include <string.h>  // for memcpy(3)
#include <stdlib.h>  // for exit(3)
#include <signal.h>  // for sigblock(2)
#include <sys/time.h>  // for gettimeofday(2)
#include <unistd.h>  // for usleep(2)

#include <playertime.h>
extern PlayerTime* GlobalTime;

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
  bzero((char*)clients,sizeof(CClientData*)*initial_size);
  size_clients = initial_size;
  num_clients = 0;

  if(!(ufds = new struct pollfd[initial_size]))
  {
    fputs("PlayerMultiClient::PlayerMultiClient(): new failed. "
          "Out of memory? bailing \n", stderr);
    exit(1);
  }
  bzero((char*)ufds,sizeof(struct pollfd)*initial_size);

  pthread_mutex_init(&rthread_client_mutex,NULL);
  pthread_mutex_init(&wthread_client_mutex,NULL);
  //pthread_mutex_init(&ufd_mutex,NULL);

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
  void* thread_return;

  pthread_cancel(writethread);
  if(pthread_join(writethread, &thread_return))
    perror("ClientManager::~ClientManager(): pthread_join()");
  
  pthread_cancel(readthread);
  if(pthread_join(readthread, &thread_return))
    perror("ClientManager::~ClientManager(): pthread_join()");

  // tear down dynamic structures here
  if(clients)
  {
    for(int i=0;i<num_clients;i++)
    {
      if(clients[i])
      {
        //printf("deleting client %d (sock %d)\n", i, clients[i]->socket);
        delete clients[i];
      }
    }
    delete clients;
  }
  if(ufds)
    delete ufds;
}

// add a client to our watch list
void ClientManager::AddClient(CClientData* client)
{
  if(!client)
    return;

  //pthread_mutex_lock(&ufd_mutex);
  //  this is only every called from main(), so we know that these mutexes
  //  will not be locked already
  pthread_mutex_lock(&rthread_client_mutex);
  pthread_mutex_lock(&wthread_client_mutex);

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
    RemoveClient(num_clients-1,true);
    RemoveBlanks(true);
  }

  pthread_mutex_unlock(&wthread_client_mutex);
  pthread_mutex_unlock(&rthread_client_mutex);
  //pthread_mutex_unlock(&ufd_mutex);
}
    
// remove a client
void ClientManager::RemoveClient(int idx, bool have_locks)
{
  if(!have_locks)
  {
    pthread_mutex_lock(&rthread_client_mutex);
    pthread_mutex_lock(&wthread_client_mutex);
  }

  delete clients[idx];
  clients[idx] = (CClientData*)NULL;

  ufds[idx].fd = -1;
  ufds[idx].events = 0;

  if(!have_locks)
  {
    pthread_mutex_unlock(&wthread_client_mutex);
    pthread_mutex_unlock(&rthread_client_mutex);
  }
}

// shift the clients and ufds down so that they're contiguous
void ClientManager::RemoveBlanks(bool have_locks)
{
  int i,j;

  if(!num_clients)
    return;

  if(!have_locks)
  {
    pthread_mutex_lock(&rthread_client_mutex);
    pthread_mutex_lock(&wthread_client_mutex);
  }

  for(i=0;i<num_clients;)
  {
    if(!clients[i])
    {
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

  if(!have_locks)
  {
    pthread_mutex_unlock(&wthread_client_mutex);
    pthread_mutex_unlock(&rthread_client_mutex);
  }
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
  int num_to_read;
  
  //pthread_mutex_lock(&ufd_mutex);
  
  // let's try this poll(2) thing (with a 100ms timeout, because we need to 
  // notice new clients added to our watch list)
  if((num_to_read = poll(ufds,num_clients,100)) == -1)
  {
    if(errno != EINTR)
    {
      perror("ClientManager::Read(): poll(2) failed:");
      //pthread_mutex_unlock(&ufd_mutex);
      return(-1);
    }
  }

  pthread_mutex_lock(&rthread_client_mutex);

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
        pthread_mutex_unlock(&rthread_client_mutex);
        RemoveClient(i,false);
        pthread_mutex_lock(&rthread_client_mutex);
      }
    }
    else if(ufds[i].revents)
    {
      if(!(ufds[i].revents & POLLHUP))
        printf("ClientManager::Read() got strange revent 0x%x for "
               "client %d; killing it\n", ufds[i].revents,i);
      pthread_mutex_unlock(&rthread_client_mutex);
      RemoveClient(i,false);
      pthread_mutex_lock(&rthread_client_mutex);
    }
  }

  pthread_mutex_unlock(&rthread_client_mutex);
  RemoveBlanks(false);
  //pthread_mutex_unlock(&ufd_mutex);
  return(0);
}

// this is a quick hack to unlock all access mutexes so that we can exit
// cleanly even when we're in the middle of opening or closing a device
void
UnlockAllClientMutexes(void* arg)
{
  ClientManager* cr = (ClientManager*)arg;

  for(int i=0;i<cr->num_clients;i++)
  {
    if(cr->clients[i])
      pthread_mutex_unlock(&(cr->clients[i]->access));
  }
}

void*
ClientReaderThread(void* arg)
{
  ClientManager* cr = (ClientManager*)arg;

  // make sure we don't get cancelled at the wrong time
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif

  pthread_cleanup_push(UnlockAllClientMutexes,arg);

  for(;;)
  {
    // should we quit?
    pthread_testcancel();

    if(cr->Read() == -1)
      pthread_exit(0);
  }

  pthread_cleanup_pop(1);
}

void*
ClientWriterThread(void* arg)
{
  ClientManager* cr = (ClientManager*)arg;
  struct timeval curr;

  // make sure we don't get cancelled at the wrong time
  pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

#ifdef PLAYER_LINUX
  sigblock(SIGINT);
  sigblock(SIGALRM);
#endif

  pthread_cleanup_push(UnlockAllClientMutexes,arg);
  
  for(;;)
  {
    // sleep a bit, then check to see if anybody needs taking care of
    //
    // NOTE: usleep(0) appears to sleep for an average of 10ms (maybe
    // we just get the scheduler delay of 10ms?).  usleep(1) sleeps for an 
    // average of 20ms (timer resolution of 10ms plus scheduler delay of 10ms?)
    //
    //usleep(1);
    usleep(0);

    if(GlobalTime->GetTime(&curr) == -1)
      fputs("CLock::PutData(): GetTime() failed!!!!\n", stderr);

    // should we quit?
    pthread_testcancel();
    
    // lock access to the array of clients
    pthread_mutex_lock(&(cr->wthread_client_mutex));

    for(int i=0;i<cr->num_clients;i++)
    {
      // maybe it's just been deleted?
      if(!cr->clients[i])
        continue;
      // lock access to the client's internal state
      pthread_mutex_lock(&(cr->clients[i]->access));
      if(cr->clients[i]->auth_pending)
      {
        pthread_mutex_unlock(&(cr->clients[i]->access));
        continue;
      }

      // is it time to write?
      if((cr->clients[i]->mode == PLAYER_DATAMODE_PUSH_ALL) || 
         (cr->clients[i]->mode == PLAYER_DATAMODE_PUSH_NEW))
      {
        if(((curr.tv_sec+(curr.tv_usec/1000000.0))-
            cr->clients[i]->last_write) + 0.005 >= 
           (1.0/cr->clients[i]->frequency))
        {
          if(cr->clients[i]->Write() == -1)
          {
            // write must have errored. dump it
            pthread_mutex_unlock(&(cr->wthread_client_mutex));
            cr->RemoveClient(i,false);
            pthread_mutex_lock(&(cr->wthread_client_mutex));
          }
          else
            cr->clients[i]->last_write = curr.tv_sec + curr.tv_usec / 1000000.0;
        }
      }
      else if(((cr->clients[i]->mode == PLAYER_DATAMODE_PULL_ALL) ||
               (cr->clients[i]->mode == PLAYER_DATAMODE_PULL_NEW))&&
              (cr->clients[i]->datarequested))
      {
        cr->clients[i]->datarequested = false;
        if(cr->clients[i]->Write() == -1)
        {
          // write must have errored. dump it
          pthread_mutex_unlock(&(cr->wthread_client_mutex));
          cr->RemoveClient(i,false);
          pthread_mutex_lock(&(cr->wthread_client_mutex));
        }
      }
      if(cr->clients[i])
        pthread_mutex_unlock(&(cr->clients[i]->access));
    }
    pthread_mutex_unlock(&(cr->wthread_client_mutex));
    cr->RemoveBlanks(false);
  }
  pthread_cleanup_pop(1);
}

