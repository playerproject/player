/* 
 *  libplayerc : a Player client library
 *  Copyright (C) Andrew Howard 2002-2003
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) Andrew Howard 2003
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/***************************************************************************
 * Desc: Multi-client functions
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>       // for gethostbyname()
#include <netinet/in.h>  // for struct sockaddr_in, htons(3)
#include <errno.h>

#include "replace.h"  /* for poll */
#include "playerc.h"
#include "error.h"


// Create a multi-client
playerc_mclient_t *playerc_mclient_create()
{
  playerc_mclient_t *mclient;

  mclient = malloc(sizeof(playerc_mclient_t));
  memset(mclient, 0, sizeof(playerc_mclient_t));
  mclient->pollfd = calloc(128,sizeof(struct pollfd));

  return mclient;
}


// Destroy a multi-client
void playerc_mclient_destroy(playerc_mclient_t *mclient)
{
  free(mclient->pollfd);
  free(mclient);
}


// Add a client to this multi-client
int playerc_mclient_addclient(playerc_mclient_t *mclient, playerc_client_t *client)
{
  if (mclient->client_count >= sizeof(mclient->client) / sizeof(mclient->client[0]))
  {
    PLAYERC_ERR("too many clients in multi-client; ignoring new client");
    return -1;
  }

  mclient->client[mclient->client_count] = client;
  mclient->client_count++;

  return 0;
}


// Test to see if there is pending data.
// Returns -1 on error, 0 or 1 otherwise.
int playerc_mclient_peek(playerc_mclient_t *mclient, int timeout)
{
  int i, count;

  // Configure poll structure to wait for incoming data 
  for (i = 0; i < mclient->client_count; i++)
  {
    mclient->pollfd[i].fd = mclient->client[i]->sock;
    mclient->pollfd[i].events = POLLIN;
    mclient->pollfd[i].revents = 0;
  }

  // Wait for incoming data 
  count = poll(mclient->pollfd, mclient->client_count, timeout);
  if (count < 0)
  {
    PLAYERC_ERR1("poll returned error [%s]", strerror(errno));
    return -1;
  }

  return (count > 0);
}


// Read from a bunch of clients
int playerc_mclient_read(playerc_mclient_t *mclient, int timeout)
{
  int i, count;

  // Configure poll structure to wait for incoming data 
  for (i = 0; i < mclient->client_count; i++)
  {
    mclient->pollfd[i].fd = mclient->client[i]->sock;
    mclient->pollfd[i].events = POLLIN;
    mclient->pollfd[i].revents = 0;
  }

  // Wait for incoming data 
  count = poll(mclient->pollfd, mclient->client_count, timeout);
  if (count < 0)
  {
    PLAYERC_ERR1("poll returned error [%s]", strerror(errno));
    return -1;
  }

  // Now read from each of the waiting sockets 
  for (i = 0; i < mclient->client_count; i++)
  {
    if ((mclient->pollfd[i].revents & POLLIN) > 0)
      playerc_client_read(mclient->client[i]);
  }
  return count;
}



