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

// Forward declarations
struct pollfd;

#include <pthread.h>

#include "clientdata.h"   

class ClientManager
{
  private:
    // A condition variable (and accompanying mutex) that can be used to
    // signal the clientmanager that new data is available
    pthread_cond_t cond;
    pthread_mutex_t condMutex;
    
  protected:
    // dynamically managed array of structs that we'll give to poll(2)
    // when trying to read from clients
    struct pollfd* ufds;
    
    // dynamically managed array of our clients
    int num_clients;
    int size_clients;
    ClientData** clients;

    // counter used to generate quasi-unique client IDs.  obviously, it can
    // roll over, but only after 64K clients have connected.
    uint16_t curr_client_id;

    // dynamically managed array of structs that we'll give to poll(2)
    // when trying to accept new connections
    struct pollfd* accept_ufds;
    int* accept_ports;
    int num_accept_ufds;

    // authorization key to be used for clients
    char client_auth_key[PLAYER_KEYLEN];

    // wait on the condition variable
    void Wait(void);

  public:
    // constructor
    ClientManager(struct pollfd* listen_ufds, int* ports,
                  int numfds, char* auth_key);

    // destructor
    virtual ~ClientManager();
    
    // add a client to our watch list
    void AddClient(ClientData* client);

    // Update the ClientManager
    int Update();

    // mark a client for deletion
    void MarkClientForDeletion(int idx);
    // remove a client
    void RemoveBlanks();
    // call Update() on all subscribed devices
    void UpdateDevices();

    // Signal that new data is available (calls pthread_cond_broadcast()
    // on the clientmanager's condition variable, which will wake it up.
    void DataAvailable(void);

    // These 3 methods are the primary interface to the ClientManager.
    // They must be implemented by any subclass.
    virtual int Accept() = 0;
    virtual int Read() = 0;
    virtual int Write() = 0;

    // get the index corresponding to a ClientData pointer
    int GetIndex(ClientData* ptr);

    // Reset 'last_write' field to 0.0 in each client.  Used when playing back
    // data from a logfile and a client requests the logfile be rewound.
    void ResetClientTimestamps(void);
    
    // Writes a message to the appropriate client queue's
    // if client == NULL then message is sent to all client subscribed to the client
    void PutMsg(uint16_t type, uint16_t device, uint16_t device_index, 
                uint32_t timestamp_sec, uint32_t timestamp_usec,
		uint32_t size, unsigned char * data, ClientData * client=NULL);
};

class ClientManagerTCP : public ClientManager
{
  public:
    ClientManagerTCP(struct pollfd* listen_ufds, int* ports,
                     int numfds, char* auth_key) :
            ClientManager(listen_ufds,ports,numfds,auth_key) {}
    virtual int Accept();
    virtual int Read();
    virtual int Write();
};

class ClientManagerUDP : public ClientManager
{
  public:
    ClientManagerUDP(struct pollfd* listen_ufds, int* ports,
                     int numfds, char* auth_key) :
            ClientManager(listen_ufds,ports,numfds,auth_key) {}

    virtual int Accept();
    virtual int Read();
    virtual int Write();

};

extern ClientManager * clientmanager;

#endif
