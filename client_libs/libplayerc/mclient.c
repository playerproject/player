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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
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
#include <errno.h>
#if !defined (WIN32)
  #include <sys/socket.h>
  #include <unistd.h>
  #include <netdb.h>       // for gethostbyname()
  #include <netinet/in.h>  // for struct sockaddr_in, htons(3)
#endif

#include <replace/replace.h>  /* for poll */

#include "playerc.h"
#include "error.h"

#if defined (WIN32)
  #define snprintf _snprintf
#endif

// Create a multi-client
playerc_mclient_t *playerc_mclient_create()
{
  playerc_mclient_t *mclient;

  mclient = malloc(sizeof(playerc_mclient_t));
  memset(mclient, 0, sizeof(playerc_mclient_t));
  mclient->pollfd = calloc(128,sizeof(struct pollfd));
  mclient->time = 0.0;

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
    playerc_client_requestdata(mclient->client[i]);
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
    if(!mclient->client[i]->qlen)
    {
      // In case the client is in a PULL mode, first request a round of data.
      if(playerc_client_requestdata(mclient->client[i]) < 0)
        PLAYERC_ERR("playerc_client_requestdata errored");
    }
  }

  // Wait for incoming data 
  count = poll(mclient->pollfd, mclient->client_count, timeout);
  if (count < 0)
  {
    PLAYERC_ERR1("poll returned error [%s]", strerror(errno));
    return -1;
  }

  // Now read from each of the waiting sockets 
  count = 0;
  for (i = 0; i < mclient->client_count; i++)
  {
    if(mclient->client[i]->qlen ||
       (mclient->pollfd[i].revents & POLLIN) > 0)
    {
      if(playerc_client_read_nonblock(mclient->client[i])>0)
      {
        // cache the latest timestamp
        if(mclient->client[i]->datatime > mclient->time)
          mclient->time = mclient->client[i]->datatime;
        count++;
      }
      else
      {
        // got no message even though poll() said there was something there, which 
        // almost certainly means that we lost the connection
        return(-1);
      }
    }
  }
  return count;
}



