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
 *   class for encapsulating all the data pertaining to the clients
 */

#ifndef _CLIENTMANAGER_H
#define _CLIENTMANAGER_H

#include <pthread.h>
#include <sys/poll.h>   // for poll(2) and associated structs
#include "clientdata.h"   

class ClientManager
{
  private:
    // dynamically managed array of structs that we'll give to poll(2)
    // when trying to read from clients
    struct pollfd* ufds;
    
    // dynamically managed array of our clients
    int num_clients;
    int size_clients;
    CClientData** clients;

    // dynamically managed array of structs that we'll give to poll(2)
    // when trying to accept new connections
    struct pollfd* accept_ufds;
    int* accept_ports;
    int num_accept_ufds;

    // authorization key to be used for clients
    char client_auth_key[PLAYER_KEYLEN];

  public:
    // constructor
    ClientManager(struct pollfd* listen_ufds, int* ports,
                  int numfds, char* auth_key);

    // destructor
    ~ClientManager();
    
    // add a client to our watch list
    void AddClient(CClientData* client);

    // Update the ClientManager
    int Update();

    // mark a client for deletion
    void MarkClientForDeletion(int idx);
    // remove a client
    void RemoveBlanks();

    int Accept();
    int Read();
    int Write();

    // get the index corresponding to a CClientData pointer
    int GetIndex(CClientData* ptr);
};

#endif
