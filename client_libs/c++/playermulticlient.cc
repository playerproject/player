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

#include <playermulticlient.h>

// constructor
PlayerMultiClient::PlayerMultiClient()
{
  // initialize both array to this size; we'll double it each time
  // we need more space later
  int initial_size = 8;

  if(!(clients = new PlayerClient*[initial_size]))
  {
    if(player_debug_level(-1)>=0)
      fputs("PlayerMultiClient::PlayerMultiClient(): new failed. "
            "Out of memory?", stderr);
    return;
  }
  size_clients = initial_size;
  num_clients = 0;

  if(!(ufds = new pollfd[initial_size]))
  {
    if(player_debug_level(-1)>=0)
      fputs("PlayerMultiClient::PlayerMultiClient(): new failed. "
            "Out of memory?", stderr);
    return;
  }
  size_ufds = initial_size;
  num_ufds = 0;
}
    
// destructor
PlayerMultiClient::~PlayerMultiClient()
{
  // tear down dynamic structures here
  if(clients)
    delete clients;
  if(ufds)
    delete ufds;
}

// add a client to our watch list
void PlayerMultiClient::AddClient(PlayerClient* client)
{
  if(!client)
    return;

  // First, add it to the array of clients

  // do we need more room?
  if(num_clients >= size_clients)
  {
    PlayerClient** tmp_clients;

    // allocate twice as much space
    if(!(tmp_clients = new PlayerClient*[2*size_clients]))
    {
      if(player_debug_level(-1) >= 0)
        fputs("PlayerMultiClient::AddClient(): new failed. Out of memory?",
              stderr);
      return;
    }

    // copy existing data
    memcpy(tmp_clients,clients,sizeof(PlayerClient*)*num_clients);

    // kill the old array
    delete clients;

    // swap pointers
    clients = tmp_clients;
    size_clients = 2*size_clients;
  }

  clients[num_clients++] = client;

  // Now, add it to our poll watchlist

  // do we need more room?
  if(num_ufds >= size_ufds)
  {
    struct pollfd* tmp_ufds;

    // allocate twice as much space
    if(!(tmp_ufds = new struct pollfd[2*size_ufds]))
    {
      if(player_debug_level(-1) >= 0)
        fputs("PlayerMultiClient::AddClient(): new failed. Out of memory?",
              stderr);
      return;
    }

    // copy existing data
    memcpy(tmp_ufds,ufds,sizeof(struct pollfd)*num_ufds);

    // kill the old array
    delete ufds;

    // swap pointers
    ufds = tmp_ufds;
    size_ufds = 2*size_ufds;
  }

  ufds[num_ufds].fd = client->conn.sock;
  ufds[num_ufds++].events = POLLIN;
}


// read from one client (the first available)
//
// Returns:
//    0 if everything went OK
//   -1 if something went wrong (you should probably close the connection!)
//
int PlayerMultiClient::Read()
{
  int num_to_read,retval;
  // let's try this poll(2) thing (with no timeout)
  if((num_to_read = poll(ufds,num_ufds,-1)) == -1)
  {
    if(player_debug_level(-1) >= 2)
      perror("PlayerMultiClient::Read(): poll(2) failed:");
    return(-1);
  }

  //printf("EVENTS: %d\n", num_to_read);
  // call the corresponding Read() for each one that's ready
  for(int i=0;i<num_ufds && num_to_read;i++)
  {
    // is this one ready to read?
    if(ufds[i].revents & POLLIN)
    {
      //printf("reading from: %d 0x%x\n", i,ufds[i].events);
      if((retval = clients[i]->Read()) == -1)
        return(retval);
    }
    else if(ufds[i].revents)
    {
      if(player_debug_level(-1) >= 3)
        printf("PlayerMultiClient::Read() got strange revent 0x%x for "
               "client %d\n", ufds[i].revents,i);
    }
  }

  return(0);
}

