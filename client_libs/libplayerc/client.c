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
 * Desc: Single-client functions
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>       // for gethostbyname()
#include <errno.h>
#include <sys/time.h>

#include <replace/replace.h>  // for poll(2)

#include "playerc.h"
#include "error.h"

// TODO: expose this timeout value somewhere
#define REQUEST_TIMEOUT 10.0

// Have we done one-time intialization work yet?
static int init_done;

// TODO: get rid of this structure
// Player message structure for subscibing to devices.  This one is
// easier to use than the one defined in messages.h.
typedef struct
{
  uint16_t subtype;
  uint16_t device;
  uint16_t index;
  uint8_t access;
} __attribute__ ((packed)) playerc_msg_subscribe_t;


// Local functions
int playerc_client_get_driverinfo(playerc_client_t *client);
int playerc_client_readpacket(playerc_client_t *client,
                              player_msghdr_t *header,
                              char *data);
int playerc_client_writepacket(playerc_client_t *client,
                               player_msghdr_t *header,
                               char *data);
void playerc_client_push(playerc_client_t *client,
                         player_msghdr_t *header, void *data);
int playerc_client_pop(playerc_client_t *client,
                       player_msghdr_t *header, void *data);
void *playerc_client_dispatch(playerc_client_t *client,
                              player_msghdr_t *header, void *data);
#if 0
void *playerc_client_dispatch_geom(playerc_client_t *client,
                              player_msghdr_t *header, void *data, int len);
void *playerc_client_dispatch_config(playerc_client_t *client,
                              player_msghdr_t *header, void *data, int len);
#endif


// Create a player client
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient, const char *host, int port)
{
  playerc_client_t *client;

  // Have we done one-time intialization work yet?
  if(!init_done)
  {
    playerxdr_ftable_init();
    init_done = 1;
  }

  client = malloc(sizeof(playerc_client_t));
  memset(client, 0, sizeof(playerc_client_t));

  client->id = client;
  client->host = strdup(host);
  client->port = port;

  if (mclient)
    playerc_mclient_addclient(mclient, client);

  // TODO: make this memory allocation more conservative
  client->data = (char*)malloc(PLAYER_MAX_MESSAGE_SIZE);
  client->xdrdata = (char*)malloc(PLAYERXDR_MAX_MESSAGE_SIZE);
  assert(client->data);
  assert(client->xdrdata);

  client->qfirst = 0;
  client->qlen = 0;
  client->qsize = sizeof(client->qitems) / sizeof(client->qitems[0]);

  client->datatime = 0;
  client->lasttime = 0;

  /* this is the server's default */
  client->mode = PLAYER_DATAMODE_PUSH_NEW;

  return client;
}


// Destroy a player client
void playerc_client_destroy(playerc_client_t *client)
{
  free(client->data);
  free(client->host);
  free(client);
  return;
}


// Connect to the server
int playerc_client_connect(playerc_client_t *client)
{
  struct hostent* entp;
  struct sockaddr_in server;
  char banner[PLAYER_IDENT_STRLEN];

  // Construct socket
  client->sock = socket(PF_INET, SOCK_STREAM, 0);
  if (client->sock < 0)
  {
    PLAYERC_ERR1("socket call failed with error [%s]", strerror(errno));
    return -1;
  }

  // Construct server address
  entp = gethostbyname(client->host);
  if (entp == NULL)
  {
    PLAYERC_ERR1("gethostbyname failed with error [%s]", strerror(errno));
    return -1;
  }
  server.sin_family = PF_INET;
  memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);
  server.sin_port = htons(client->port);

  // Connect the socket
  if (connect(client->sock, (struct sockaddr*) &server, sizeof(server)) < 0)
  {
    PLAYERC_ERR3("connect call on [%s:%d] failed with error [%s]",
                 client->host, client->port, strerror(errno));
    return -1;
  }

  // Get the banner
  if (recv(client->sock, banner, sizeof(banner), 0) < sizeof(banner))
  {
    PLAYERC_ERR("incomplete initialization string");
    return -1;
  }

#if 0
  // Default to async data mode
  if (playerc_client_datamode(client, PLAYERC_DATAMODE_PUSH_ASYNC) != 0)
  {
    PLAYERC_ERR("unable to set push_async data mode");
    return -1;
  }
#endif
  // PLAYERC_MSG3("[%s] connected on [%s:%d]", banner, client->hostname, client->port);
  return 0;
}


// Disconnect from the server
int playerc_client_disconnect(playerc_client_t *client)
{
  if (close(client->sock) < 0)
  {
    PLAYERC_ERR1("close failed with error [%s]", strerror(errno));
    return -1;
  }
  return 0;
}

// add a replace rule the the clients queue on the server
int playerc_client_add_replace_rule(playerc_client_t *client, int interf, int index, int type, int subtype, int replace)
{
  player_add_replace_rule_req_t req;

  req.interf = interf;
  req.index = index;
  req.type = type;
  req.subtype = subtype;
  req.replace = replace;



  if (playerc_client_request(client, NULL, PLAYER_PLAYER_REQ_ADD_REPLACE_RULE, &req, NULL, 0) < 0)
    return -1;

  return 0;
}

#if 0
// Change the server's data delivery mode
int playerc_client_datamode(playerc_client_t *client, int mode)
{
  player_device_datamode_req_t req;

//  req.subtype = htons(PLAYER_PLAYER_DATAMODE_REQ);
  req.mode = mode;

  if (playerc_client_request(client, NULL, PLAYER_PLAYER_DATAMODE, &req, sizeof(req), NULL, 0) < 0)
    return -1;

  /* cache the change */
  client->mode = mode;

  return 0;
}

// Change the server's data delivery frequency (freq is in Hz)
int playerc_client_datafreq(playerc_client_t *client, int freq)
{
  player_device_datafreq_req_t req;

//  req.subtype = htons(PLAYER_PLAYER_DATAFREQ_REQ);
  req.frequency = htons(freq);

  if (playerc_client_request(client, NULL, PLAYER_PLAYER_DATAFREQ,  &req, sizeof(req), NULL, 0) < 0)
    return -1;

  return 0;
}

// Request a round of data; only valid when in a request/reply
// (aka PULL) mode
int
playerc_client_requestdata(playerc_client_t* client)
{
  player_device_data_req_t req;

//  req.subtype = htons(PLAYER_PLAYER_DATA_REQ);
  return(playerc_client_request(client, NULL, PLAYER_PLAYER_DATA, &req, sizeof(req), NULL, 0));
}

#endif

// Test to see if there is pending data.
int playerc_client_peek(playerc_client_t *client, int timeout)
{
  int count;
  struct pollfd fd;

  playerc_client_item_t *item;

  if (client->qlen > 0)
  {
    item = client->qitems + client->qfirst;
    return(item->header.size);
  }

  fd.fd = client->sock;
  fd.events = POLLIN | POLLHUP;
  fd.revents = 0;

  // Wait for incoming data
  count = poll(&fd, 1, timeout);
  if (count < 0)
  {
    PLAYERC_ERR1("poll returned error [%s]", strerror(errno));
    return -1;
  }
  if (count > 0 && (fd.revents & POLLHUP))
  {
    PLAYERC_ERR("socket disconnected");
    return -1;
  }
  return count;
}

// Read and process a packet (blocking)
void *playerc_client_read(playerc_client_t *client)
{
  player_msghdr_t header;

  // See if there is any queued data.
  if (playerc_client_pop(client, &header, client->data) < 0)
  {
    // If there is no queued data, read a packet (blocking).

#if 0
    // If we're in a PULL mode, first request a round of data.
    if((client->mode == PLAYER_DATAMODE_PULL_NEW) ||
       (client->mode == PLAYER_DATAMODE_PULL_ALL))
    {
       if(playerc_client_requestdata(client) < 0)
         return NULL;
    }
#endif

    if (playerc_client_readpacket(client, &header, client->data) < 0)
      return NULL;
  }

  switch(header.type)
  {
    case PLAYER_MSGTYPE_SYNCH:
      client->lasttime = client->datatime;
      client->datatime = header.timestamp;
      return client->id;

    case PLAYER_MSGTYPE_DATA:
      return playerc_client_dispatch(client, &header, client->data);
  }

  PLAYERC_WARN1("unexpected message type [%d]", header.type);
  return NULL;
}


// Write a command
int playerc_client_write(playerc_client_t *client,
                         playerc_device_t *device,
                         uint8_t subtype,
                         void *cmd, double* timestamp)
{
  player_msghdr_t header;
  struct timeval curr;

  memset(&header,0,sizeof(player_msghdr_t));

  header.addr = device->addr;
  header.type = PLAYER_MSGTYPE_CMD;
  header.subtype = subtype;
  if(timestamp)
    header.timestamp = *timestamp;
  else
  {
    gettimeofday(&curr,NULL);
    header.timestamp = curr.tv_sec + curr.tv_usec/1e6;
  }

  return playerc_client_writepacket(client, &header, cmd);
}

// Issue request and await reply (blocking).
int playerc_client_request(playerc_client_t *client,
                           playerc_device_t *deviceinfo,
                           uint8_t subtype,
                           void *req_data, void *rep_data, int rep_len)
{
  double t;
  struct timeval last;
  struct timeval curr;
  player_msghdr_t req_header, rep_header;

  if(deviceinfo == NULL)
  {
    req_header.addr.host = 0;
    req_header.addr.robot = 0;
    req_header.addr.interf = PLAYER_PLAYER_CODE;
    req_header.addr.index = 0;
    req_header.type = PLAYER_MSGTYPE_REQ;
  }
  else
  {
    req_header.addr = deviceinfo->addr;
    req_header.type = PLAYER_MSGTYPE_REQ;
  }
  req_header.subtype = subtype;


  if (playerc_client_writepacket(client, &req_header, req_data) < 0)
    return -1;

  t = REQUEST_TIMEOUT;

  // Read packets until we get a reply.  Data packets get queued up
  // for later processing.
  while(t >= 0)
  {
    int peek;
    gettimeofday(&last,NULL);
    peek = playerc_client_peek(client,10);
    gettimeofday(&curr,NULL);
    t -= ((curr.tv_sec + curr.tv_usec/1e6) - 
          (last.tv_sec + last.tv_usec/1e6));

    if(peek < 0)
      return -1;
    else if(peek == 0)
      continue;
      
    if (playerc_client_readpacket(client, &rep_header, client->data) < 0)
      return -1;

    if (rep_header.type == PLAYER_MSGTYPE_DATA)
    {
      // Queue up any incoming data packets for later dispatch
      playerc_client_push(client, &rep_header, client->data);
    }
    else if(rep_header.type == PLAYER_MSGTYPE_RESP_ACK)
    {
      // Using TCP, we only need to check the interface and index
      if (rep_header.addr.interf != req_header.addr.interf ||
          rep_header.addr.index != req_header.addr.index ||
          rep_header.subtype != req_header.subtype)
      {
        PLAYERC_ERR("got the wrong kind of reply (not good).");
        return -1;
      }
      else if(rep_header.size > rep_len)
      {
        PLAYERC_ERR2("insufficient space to store the reply (%u > %u)",
                     rep_header.size, rep_len);
        return -1;
      }
      memcpy(rep_data, client->data, rep_header.size);
      return(0);
    }
    else if (rep_header.type == PLAYER_MSGTYPE_RESP_NACK)
    {
      // Using TCP, we only need to check the interface and index
      if (rep_header.addr.interf != req_header.addr.interf ||
          rep_header.addr.index != req_header.addr.index ||
          rep_header.subtype != req_header.subtype)
      {
        PLAYERC_ERR("got the wrong kind of reply (not good).");
        return -1;
      }
      PLAYERC_ERR("got NACK from request");
      return -2;
    }
  }

  PLAYERC_ERR("timed out waiting for server reply to request");
  return -1;
}

#if 0
// Issue request and await reply (blocking).
int playerc_client_request(playerc_client_t *client,
                           playerc_device_t *deviceinfo, uint8_t reqtype,
                           void *req_data, int req_len, void *rep_data,
                           int rep_len)
{
  int resptype;
  player_msghdr_t req_header;

  req_header.stx = PLAYER_STXX;
  req_header.type = PLAYER_MSGTYPE_REQ;
  req_header.subtype = reqtype;
  req_header.size = req_len;

  if (deviceinfo == NULL)
  {
    req_header.device = PLAYER_PLAYER_CODE;
    req_header.device_index = 0;
  }
  else
  {
    req_header.device = deviceinfo->code;
    req_header.device_index = deviceinfo->index;
  }

  if (playerc_client_writepacket(client, &req_header, req_data, req_len) < 0)
    return -1;

  resptype = playerc_client_getresponse(client, req_header.device,
                                        req_header.device_index, 0, NULL,
                                        rep_data, rep_len);
  return resptype;
}


int playerc_client_getresponse(playerc_client_t *client, uint16_t device,
                               uint16_t  index, uint16_t sequence,
                               uint8_t * resptype, uint8_t * resp_data,
                               int resp_len)
{
  player_msghdr_t rep_header;
  playerc_client_item_t *item;
  int i;
  int len;
  // First we check through the stored messages to see if one matches
  for(i = 0; i < client->qlen; ++i)
  {
    item = &client->qitems[(i + client->qfirst) % client->qsize];
    // need to add sequence to list to be checked
    if (item->header.device == device && item->header.device_index == index)
    {
      if (resptype)
        *resptype = item->header.subtype;
      // need to deal with removing items from the queue
      // circular buffer is not best option for this
      // so either need to use linked list
      // or just tag items as read and remove in read method
      if (item->header.type == PLAYER_MSGTYPE_RESP_NACK)
        return -1;
      else if (item->header.type == PLAYER_MSGTYPE_RESP_ACK)
      {
        if (item->len > resp_len)
          return -2;
        memcpy(resp_data, item->data, item->len);
        return item->len;
      }
    }
  }

  // then start reading in new messages, store in queue if they are
  // not the one we are waiting for
  for (i = 0; i < 1000; i++)
  {
    len = PLAYER_MAX_MESSAGE_SIZE;
    if (playerc_client_readpacket(client, &rep_header, client->data, &len) < 0)
      return -1;

    // need to add sequence to list to be checked
    if (rep_header.device == device && rep_header.device_index == index)
    {
      if (resptype)
        *resptype = rep_header.subtype;
      // need to deal with removing items from the queue
      // circular buffer is not best option for this
      // so either need to use linked list
      // or just tag items as read and remove in read method
      if (rep_header.type == PLAYER_MSGTYPE_RESP_NACK)
        return -1;
      else if (rep_header.type == PLAYER_MSGTYPE_RESP_ACK)
      {
        if (len > resp_len)
          return -2;
        memcpy(resp_data, client->data, len);
        return len;
      }
    }
    // Queue up any incoming data packets for later dispatch
    playerc_client_push(client, &rep_header, client->data, len);
  }

  PLAYERC_ERR("timed out waiting for server reply to request");
  return -1;
}
#endif


// Add a device proxy
int playerc_client_adddevice(playerc_client_t *client, playerc_device_t *device)
{
  if (client->device_count >= sizeof(client->device) / sizeof(client->device[0]))
  {
    PLAYERC_ERR("too many devices");
    return -1;
  }
  device->fresh = 0;
  client->device[client->device_count++] = device;
  return 0;
}


// Remove a device proxy
int playerc_client_deldevice(playerc_client_t *client, playerc_device_t *device)
{
  int i;

  for (i = 0; i < client->device_count; i++)
  {
    if (client->device[i] == device)
    {
      memmove(client->device + i, client->device + i + 1,
              (client->device_count - i - 1) * sizeof(client->device[0]));
      client->device_count--;
      return 0;
    }
  }
  PLAYERC_ERR("unknown device");
  return -1;
}


// Get the list of available device ids.  The data is written into the
// proxy structure rather than returned to the caller.
int playerc_client_get_devlist(playerc_client_t *client)
{
  int i;
  player_device_devlist_t config;

  memset(&config,0,sizeof(config));

  if(playerc_client_request(client, NULL, PLAYER_PLAYER_REQ_DEVLIST,
                            &config, &config, sizeof(config)) < 0)
  {
    PLAYERC_ERR("failed to get response");
    return(-1);
  }

  for (i = 0; i < config.devices_count; i++)
    client->devinfos[i].addr = config.devices[i];
  client->devinfo_count = config.devices_count;

  // Now get the driver info
  return playerc_client_get_driverinfo(client);
}


// Get the driver info for all devices.  The data is written into the
// proxy structure rather than returned to the caller.
int playerc_client_get_driverinfo(playerc_client_t *client)
{
  int i;
  player_device_driverinfo_t req;

  for (i = 0; i < client->devinfo_count; i++)
  {
    memset(&req,0,sizeof(req));
    req.addr = client->devinfos[i].addr;

    if(playerc_client_request(client, NULL, PLAYER_PLAYER_REQ_DRIVERINFO,
                              &req, &req, sizeof(req)) < 0)
    {
      PLAYERC_ERR("failed to get response");
      return(-1);
    }

    strncpy(client->devinfos[i].drivername, req.driver_name,
            req.driver_name_count);
    client->devinfos[i].drivername[req.driver_name_count] = '\0';
  }

  return 0;
}


// Subscribe to a device
int playerc_client_subscribe(playerc_client_t *client, int code, int index,
                             int access, char *drivername, size_t len)
{
  player_device_req_t req;

  req.addr.host = 0;
  req.addr.robot = 0;
  req.addr.interf = code;
  req.addr.index = index;
  req.access = access;
  req.driver_name_count = 0;

  if (playerc_client_request(client, NULL, PLAYER_PLAYER_REQ_DEV,
                             (void*)&req, (void*)&req, sizeof(req)) < 0)
  {
    PLAYERC_ERR("failed to get response");
    return -1;
  }

  if (req.access != access)
  {
    PLAYERC_ERR2("requested [%d] access, but got [%d] access", access, req.access);
    return -1;
  }

  // Copy the driver name
  strncpy(drivername, req.driver_name, len);

  return 0;
}


// Unsubscribe from a device
int playerc_client_unsubscribe(playerc_client_t *client, int code, int index)
{
  player_device_req_t req;

  req.addr.host = 0;
  req.addr.robot = 0;
  req.addr.interf = code;
  req.addr.index = index;
  req.access = PLAYER_CLOSE_MODE;
  req.driver_name_count = 0;

  if (playerc_client_request(client, NULL, PLAYER_PLAYER_REQ_DEV,
                             (void*)&req, (void*)&req, sizeof(req)) < 0)
    return -1;

  if (req.access != PLAYER_CLOSE_MODE)
  {
    PLAYERC_ERR2("requested [%d] access, but got [%d] access", PLAYER_CLOSE_MODE, req.access);
    return -1;
  }

  return 0;
}


// Register a callback.  Will be called when after data has been read
// by the indicated device.
int playerc_client_addcallback(playerc_client_t *client, playerc_device_t *device,
                               playerc_callback_fn_t callback, void *data)
{
  if (device->callback_count >= sizeof(device->callback) / sizeof(device->callback[0]))
  {
    PLAYERC_ERR("too many registered callbacks; ignoring new callback");
    return -1;
  }
  device->callback[device->callback_count] = callback;
  device->callback_data[device->callback_count] = data;
  device->callback_count++;

  return 0;
}


// Unregister a callback
int playerc_client_delcallback(playerc_client_t *client, playerc_device_t *device,
                               playerc_callback_fn_t callback, void *data)
{
  int i;

  for (i = 0; i < device->callback_count; i++)
  {
    if (device->callback[i] != callback)
      continue;
    if (device->callback_data[i] != data)
      continue;
    memmove(device->callback + i, device->callback + i + 1,
            (device->callback_count - i - 1) * sizeof(device->callback[0]));
    memmove(device->callback_data + i, device->callback_data + i + 1,
            (device->callback_count - i - 1) * sizeof(device->callback_data[0]));
    device->callback_count--;
  }
  return 0;
}


// Read a raw packet
int playerc_client_readpacket(playerc_client_t *client,
                              player_msghdr_t *header,
                              char *data)
{
  int nbytes, bytes, total_bytes;
  player_pack_fn_t packfunc;
  int decode_msglen;

  // Read header
  for (bytes = 0; bytes < PLAYERXDR_MSGHDR_SIZE;)
  {
    nbytes = recv(client->sock, client->xdrdata + bytes,
                  PLAYERXDR_MSGHDR_SIZE - bytes, 0);
    if (nbytes <= 0)
    {
      PLAYERC_ERR1("recv on header failed with error [%s]", strerror(errno));
      return -1;
    }
    bytes += nbytes;
  }
  if (bytes < PLAYERXDR_MSGHDR_SIZE)
  {
    PLAYERC_ERR2("got incomplete header, %d of %u bytes",
                 bytes, PLAYERXDR_MSGHDR_SIZE);
    return -1;
  }

  // Unpack the header
  if(player_msghdr_pack(client->xdrdata,
                        PLAYERXDR_MSGHDR_SIZE,
                        header, PLAYERXDR_DECODE) < 0)
  {
    PLAYERC_ERR("failed to unpack header");
    return -1;
  }

  if (header->size > PLAYERXDR_MAX_MESSAGE_SIZE - PLAYERXDR_MSGHDR_SIZE)
  {
    PLAYERC_ERR1("packet is too large, %d bytes", header->size);
    return -1;
  }

  // Read in the body of the packet
  total_bytes = 0;
  while (total_bytes < header->size)
  {
    bytes = recv(client->sock,
                 client->xdrdata + PLAYERXDR_MSGHDR_SIZE + total_bytes,
                 header->size - total_bytes, 0);
    if (bytes <= 0)
    {
      PLAYERC_ERR1("recv on body failed with error [%s]", strerror(errno));
      return -1;
    }
    total_bytes += bytes;
  }

  // Locate the appropriate unpacking function for the message body
  if(!(packfunc = playerxdr_get_func(header->addr.interf, header->type,
                                     header->subtype)))
  {
    // TODO: Allow the user to register a callback to handle unsupported
    // messages
    PLAYERC_ERR3("skipping message from %u:%u with unsupported type %u",
                 header->addr.interf, header->addr.index, header->subtype);
    return(-1);
  }

  // Unpack the body
  if((decode_msglen = (*packfunc)(client->xdrdata + PLAYERXDR_MSGHDR_SIZE,
                                  header->size, data, PLAYERXDR_DECODE)) < 0)
  {
    PLAYERC_ERR3("deconding failed on message from %u:%u with type %u",
                 header->addr.interf, header->addr.index, header->subtype);
    return(-1);
  }

  // Rewrite the header with the decoded message length
  header->size = decode_msglen;

  return 0;
}


// Write a raw packet
int playerc_client_writepacket(playerc_client_t *client,
                               player_msghdr_t *header, char *data)
{
  int bytes;
  player_pack_fn_t packfunc;
  int encode_msglen;
  struct timeval curr;

  // Encode the body first, if it's non-NULL
  if(data)
  {
    // Locate the appropriate packing function for the message body
    if(!(packfunc = playerxdr_get_func(header->addr.interf,
                                       header->type,
                                       header->subtype)))
    {
      // TODO: Allow the user to register a callback to handle unsupported
      // messages
      PLAYERC_ERR3("skipping message to %u:%u with unsupported type %u",
                   header->addr.interf, header->addr.index, header->subtype);
      return(-1);
    }

    if((encode_msglen =
        (*packfunc)(client->xdrdata + PLAYERXDR_MSGHDR_SIZE,
                    PLAYER_MAX_MESSAGE_SIZE - PLAYERXDR_MSGHDR_SIZE,
                    data, PLAYERXDR_ENCODE)) < 0)
    {
      PLAYERC_ERR4("encoding failed on message from %u:%u with type %u:%u",
                   header->addr.interf, header->addr.index, header->type, header->subtype);
      return(-1);
    }
  }
  else
    encode_msglen = 0;

  // Write in the encoded size and current time
  header->size = encode_msglen;
  gettimeofday(&curr,NULL);
  header->timestamp = curr.tv_sec + curr.tv_usec / 1e6;
  // Pack the header
  if(player_msghdr_pack(client->xdrdata, PLAYERXDR_MSGHDR_SIZE,
                        header, PLAYERXDR_ENCODE) < 0)
  {
    PLAYERC_ERR("failed to pack header");
    return -1;
  }

  // Send the message
  bytes = send(client->sock, client->xdrdata,
               PLAYERXDR_MSGHDR_SIZE + encode_msglen, 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("send on body failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < (PLAYERXDR_MSGHDR_SIZE + encode_msglen))
  {
    PLAYERC_ERR2("sent incomplete message, %d of %d bytes",
                 bytes, PLAYERXDR_MSGHDR_SIZE + encode_msglen);
    return -1;
  }

  return 0;
}


// Push a packet onto the incoming queue.
void playerc_client_push(playerc_client_t *client,
                         player_msghdr_t *header, void *data)
{
  playerc_client_item_t *item;

  // Check for queue overflow; this will leak mem.
  if (client->qlen == client->qsize)
  {
    PLAYERC_ERR("queue overflow; discarding packets");
    client->qfirst = (client->qfirst + 1) % client->qsize;
    client->qlen -=1;
  }

  item = client->qitems + (client->qfirst + client->qlen) % client->qsize;
  item->header = *header;
  item->data = malloc(header->size);
  memcpy(item->data, data, header->size);

  client->qlen +=1;

  return;
}


// Pop a packet from the incoming queue.  Returns non-zero if the
// queue is empty.
int playerc_client_pop(playerc_client_t *client,
                       player_msghdr_t *header, void *data)
{
  playerc_client_item_t *item;

  if (client->qlen == 0)
    return -1;

  item = client->qitems + client->qfirst;
  *header = item->header;
  memcpy(data, item->data, header->size);
  free(item->data);

  client->qfirst = (client->qfirst + 1) % client->qsize;
  client->qlen -= 1;

  return(0);
}


// Dispatch a packet
void *playerc_client_dispatch(playerc_client_t *client,
                              player_msghdr_t *header,
                              void *data)
{
  int i, j;
  playerc_device_t *device;

  // Look for a device proxy to handle this data
  for (i = 0; i < client->device_count; i++)
  {
    device = client->device[i];

    if (device->addr.interf == header->addr.interf &&
        device->addr.index == header->addr.index)
    {
      // Fill out timing info
      device->lasttime = device->datatime;
      device->datatime = header->timestamp;

      // Call the registerd handler for this device
      if(device->putmsg)
      {
        (*device->putmsg) (device, (char*) header, data);

        // mark as fresh
        device->fresh = 1;

        // Call any additional registered callbacks
        for (j = 0; j < device->callback_count; j++)
          (*device->callback[j]) (device->callback_data[j]);

        return device->id;
      }
      else
        return NULL;
    }
  }
  return NULL;
}

