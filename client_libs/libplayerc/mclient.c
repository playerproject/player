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

#include "playerc.h"
#include "error.h"


// Create a multi-client
playerc_mclient_t *playerc_mclient_create()
{
  playerc_mclient_t *mclient;

  mclient = malloc(sizeof(playerc_mclient_t));
  memset(mclient, 0, sizeof(playerc_mclient_t));

  return mclient;
}


// Destroy a multi-client
void playerc_mclient_destroy(playerc_mclient_t *mclient)
{
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


// Connect a bunch of clients
int playerc_mclient_connect(playerc_mclient_t *mclient)
{
  int i;

  for (i = 0; i < mclient->client_count; i++)
  {
    if (playerc_client_connect(mclient->client[i]) < 0)
      return -1;
  }
  return 0;
}


// Disconnect a bunch of clients
int playerc_mclient_disconnect(playerc_mclient_t *mclient)
{
  int i;

  for (i = 0; i < mclient->client_count; i++)
  {
    if (playerc_client_disconnect(mclient->client[i]) < 0)
      return -1;
  }
  return 0;
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


