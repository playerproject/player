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
 *   class for encapsulating all the data pertaining to the client reader
 *   and writer threads
 */

#ifndef _CLIENTMANAGER_H
#define _CLIENTMANAGER_H

#include <pthread.h>
#include <sys/poll.h>   // for poll(2) and associated structs
#include "clientdata.h"   

class ClientManager
{
  private:
    pthread_t readthread;
    pthread_t writethread;
    

    // dynamically managed array of structs that we'll give to poll(2)
    struct pollfd* ufds;

  public:
    // dynamically managed array of our clients
    int num_clients;
    int size_clients;
    CClientData** clients;

    // ClientReaderThread locks these two:
    pthread_mutex_t rthread_client_mutex;
    //pthread_mutex_t ufd_mutex;
    
    // ClientWriterThread locks this one:
    pthread_mutex_t wthread_client_mutex;

    
    // constructor
    ClientManager();

    // destructor
    ~ClientManager();
    
    // add a client to our watch list
    void AddClient(CClientData* client);

    // mark a client for deletion
    void MarkClientForDeletion(int idx, bool have_lock);
    // remove a client
    void RemoveClient(int idx, bool have_locks);
    void RemoveBlanks(bool have_locks);

    int Read();
    int Write();

    // get the index corresponding to a CClientData pointer
    int GetIndex(CClientData* ptr);
};

#endif
