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
 * multiclient extension to the c++ client.
 */

#ifndef PLAYERMULTICLIENT_H
#define PLAYERMULTICLIENT_H

#include <playerclient.h>   // the normal C++ client
#include <sys/poll.h>   // for poll(2) and associated structs

class PlayerMultiClient
{
  private:
    // dynamically managed array of our clients
    PlayerClient** clients;
    //int num_clients;
    int size_clients;

    // dynamically managed array of structs that we'll give to poll(2)
    struct pollfd* ufds;
    int size_ufds;
    int num_ufds;

  public:
    // constructor
    PlayerMultiClient();
    
    // destructor
    ~PlayerMultiClient();

    // how many clients are we managing?
    int GetNumClients( void ){ return num_ufds; };

    // return a client pointer from a host and port, zero if not connected 
    // to that address
    PlayerClient* GetClient( char* host, int port )
      {
	//printf( "searching for %s:%d\n",
	//host, port );
	for( int c=0; c<num_ufds; c++ )
	  {
	    //printf( "checking [%d] %s:%d\n",
	    //    c, clients[c]->hostname, clients[c]->port );

	  if( (strcmp(clients[c]->hostname, host) == 0 ) 
	      && clients[c]->port == port )
	    return clients[c];
	  }
	return 0;
      };
    
    
    // add a client to our watch list
    void AddClient(PlayerClient* client);
    
    // remove a client from the watch list
    void RemoveClient(PlayerClient* client);

    // read from one client (the first available)
    //
    // Returns:
    //    0 if everything went OK
    //   -1 if something went wrong (you should probably close the connection!)
    //
    int Read();

    // same as Read(), but reads everything off the socket so we end
    // up with the freshest data, subject to N maximum reads
    int ReadLatest( int max_reads );
};

#endif
