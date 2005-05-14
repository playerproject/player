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
#include <sys/socket.h>
#include <netdb.h>       // for gethostbyname()
#include <netinet/in.h>  // for struct sockaddr_in, htons(3)
#include <errno.h>

#include "replace.h"  // for poll(2)
#include "playerc.h"
#include "error.h"

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
int playerc_client_readpacket(playerc_client_t *client, player_msghdr_t *header,
                              char *data, int *len);
int playerc_client_writepacket(playerc_client_t *client, player_msghdr_t *header,
                               char *data, int len);
void playerc_client_push(playerc_client_t *client,
                         player_msghdr_t *header, void *data, int len);
int playerc_client_pop(playerc_client_t *client,
                       player_msghdr_t *header, void *data, int *len);
void *playerc_client_dispatch(playerc_client_t *client,
                              player_msghdr_t *header, void *data, int len);
void *playerc_client_dispatch_geom(playerc_client_t *client,
                              player_msghdr_t *header, void *data, int len);
void *playerc_client_dispatch_config(playerc_client_t *client,
                              player_msghdr_t *header, void *data, int len);


// Create a player client
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient, const char *host, int port)
{
  playerc_client_t *client;

  client = malloc(sizeof(playerc_client_t));
  memset(client, 0, sizeof(playerc_client_t));

  client->id = client;
  client->host = strdup(host);
  client->port = port;

  if (mclient)
    playerc_mclient_addclient(mclient, client);

  client->data = (char*) malloc(PLAYER_MAX_MESSAGE_SIZE);
  
  client->qfirst = 0;
  client->qlen = 0;
  client->qsize = sizeof(client->qitems) / sizeof(client->qitems[0]);

  client->datatime = 0;

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
  char banner[32+sizeof(player_msghdr_t)];

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

  // Default to async data mode
  if (playerc_client_datamode(client, PLAYERC_DATAMODE_PUSH_ASYNC) != 0)
  {
    PLAYERC_ERR("unable to set push_async data mode");
    return -1;
  }

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


// Test to see if there is pending data.
int playerc_client_peek(playerc_client_t *client, int timeout)
{
  int count;
  struct pollfd fd;
  
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

// Request a round of data; only valid when in a request/reply 
// (aka PULL) mode
int
playerc_client_requestdata(playerc_client_t* client)
{
  player_device_data_req_t req;

//  req.subtype = htons(PLAYER_PLAYER_DATA_REQ);
  return(playerc_client_request(client, NULL, PLAYER_PLAYER_DATA, &req, sizeof(req), NULL, 0));
}

// Read and process a packet (blocking)
void *playerc_client_read(playerc_client_t *client)
{
  player_msghdr_t header;
  int len;

  // See if there is any queued data.
  len = PLAYER_MAX_MESSAGE_SIZE;
  if (playerc_client_pop(client, &header, client->data, &len) < 0)
  {
    // If there is no queued data, read a packet (blocking).

    // If we're in a PULL mode, first request a round of data.
    if((client->mode == PLAYER_DATAMODE_PULL_NEW) ||
       (client->mode == PLAYER_DATAMODE_PULL_ALL))
    {
       if(playerc_client_requestdata(client) < 0)
         return NULL;
    }

    len = PLAYER_MAX_MESSAGE_SIZE;
    if (playerc_client_readpacket(client, &header, client->data, &len) < 0)
      return NULL;
  }
  
  switch(header.type)
  {
    case PLAYER_MSGTYPE_SYNCH:
	  client->datatime = header.timestamp_sec + header.timestamp_usec * 1e-6;
      return client->id;
	
/*	case PLAYER_MSGTYPE_GEOM:
	  return playerc_client_dispatch_geom(client, &header, client->data, len);
  
  	case PLAYER_MSGTYPE_CONFIG:
	  return playerc_client_dispatch_config(client, &header, client->data, len);  
  */
  	case PLAYER_MSGTYPE_DATA:
  	{
		return playerc_client_dispatch(client, &header, client->data, len);
  	}
  }
  
	PLAYERC_WARN1("unexpected message type [%d]", header.type);
	return NULL;
}


// Write a command
// TODO: expose the subtype here.
int playerc_client_write(playerc_client_t *client, playerc_device_t *device,
                         void *cmd, int len)
{
  player_msghdr_t header;

  memset(&header,0,sizeof(player_msghdr_t));

  header.stx = PLAYER_STXX;
  header.type = PLAYER_MSGTYPE_CMD;
  header.device = device->code;
  header.device_index = device->index;
  header.size = len;

  return playerc_client_writepacket(client, &header, cmd, len);
}

// Issue request and await reply (blocking).
int playerc_client_request(playerc_client_t *client, 
                           playerc_device_t *deviceinfo,
                           uint8_t subtype,
                           void *req_data, int req_len, 
                           void *rep_data, int rep_len)
{
  int i, len;
  player_msghdr_t req_header, rep_header;

  if(deviceinfo == NULL)
  {
    req_header.stx = PLAYER_STXX;
    req_header.type = PLAYER_MSGTYPE_REQ;
    req_header.device = PLAYER_PLAYER_CODE;    
    req_header.device_index = 0;
    req_header.size = req_len;
  }
  else
  {
    req_header.stx = PLAYER_STXX;
    req_header.type = PLAYER_MSGTYPE_REQ;
    req_header.device = deviceinfo->code;
    req_header.device_index = deviceinfo->index;
    req_header.size = req_len;
  }
  req_header.subtype = subtype;

  if (playerc_client_writepacket(client, &req_header, req_data, req_len) < 0)
    return -1;

  // Read packets until we get a reply.  Data packets get queued up
  // for later processing.
  for (i = 0; i < 1000; i++)
  {
    len = PLAYER_MAX_MESSAGE_SIZE;
    if (playerc_client_readpacket(client, &rep_header, client->data, &len) < 0)
      return -1;

    if (rep_header.type == PLAYER_MSGTYPE_DATA)
    {
      // Queue up any incoming data packets for later dispatch
      playerc_client_push(client, &rep_header, client->data, len);
    }
    else if(rep_header.type == PLAYER_MSGTYPE_RESP_ACK)
    {
      if (rep_header.device != req_header.device ||
          rep_header.device_index != req_header.device_index ||
          rep_header.subtype != req_header.subtype ||
          rep_header.size > rep_len)
      {
        PLAYERC_ERR("got the wrong kind of reply (not good).");
        return -1;
      }
      memcpy(rep_data, client->data, rep_len);
      break;
    }
    else if (rep_header.type == PLAYER_MSGTYPE_RESP_NACK)
    {
      if (rep_header.device != req_header.device ||
          rep_header.device_index != req_header.device_index ||
          rep_header.subtype != req_header.subtype ||
          rep_header.size > rep_len)
      {
        PLAYERC_ERR("got the wrong kind of reply (not good).");
        return -1;
      }
      PLAYERC_ERR("got NACK from request");
      return -2;
    }
    else if (rep_header.type == PLAYER_MSGTYPE_RESP_ERR)
    {
      PLAYERC_ERR("got ERR from request");
      return -1;
    }
  }

  if (i == 1000)
  {
    PLAYERC_ERR("timed out waiting for server reply to request");
    return -1;
  }
    
  return len;
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
  int i, len;
  player_device_devlist_t config;

//  config.subtype = htons(PLAYER_PLAYER_DEVLIST_REQ);

  len = playerc_client_request(client, NULL, PLAYER_PLAYER_DEVLIST,
                               &config, sizeof(config), &config, sizeof(config));
  if (len < 0)
    return -1;
  if (len != sizeof(config))
  {
    PLAYERC_ERR2("reply to devlist request has incorrect length (%d != %d)", len, sizeof(config));
    return -1;
  }

  // Do some byte swapping.
  config.device_count = ntohs(config.device_count);
  for (i = 0; i < config.device_count; i++)
  {
    client->devinfos[i].port = ntohs(config.devices[i].port);
    client->devinfos[i].code = ntohs(config.devices[i].code);
    client->devinfos[i].index = ntohs(config.devices[i].index);
  }
  client->devinfo_count = config.device_count;

  // Now get the driver info
  return playerc_client_get_driverinfo(client);
}


// Get the driver info for all devices.  The data is written into the
// proxy structure rather than returned to the caller.
int playerc_client_get_driverinfo(playerc_client_t *client)
{
  int i, len;
  player_device_driverinfo_t req;
  player_device_driverinfo_t rep;

  for (i = 0; i < client->devinfo_count; i++)
  {
    //req.subtype = htons(PLAYER_PLAYER_DRIVERINFO_REQ);
    req.id.port = htons(client->devinfos[i].port);
    req.id.code = htons(client->devinfos[i].code);
    req.id.index = htons(client->devinfos[i].index);

    len = playerc_client_request(client, NULL, PLAYER_PLAYER_DRIVERINFO,
                                 &req, sizeof(req), &rep, sizeof(rep));
    if (len < 0)
      return -1;
    if (len != sizeof(rep))
    {
      PLAYERC_ERR2("reply to driverinfo request has incorrect length (%d != %d)",
                   len, sizeof(rep));
      return -1;
    }

    strcpy(client->devinfos[i].drivername, rep.driver_name);
  }

  return 0;
}


// Subscribe to a device
int playerc_client_subscribe(playerc_client_t *client, int code, int index,
                             int access, char *drivername, size_t len)
{
  player_device_req_t req;
  player_device_resp_t rep;

//  req.subtype = htons(PLAYER_PLAYER_DEV_REQ);
  req.code = htons(code);
  req.index = htons(index);
  req.access = access;

  if (playerc_client_request(client, NULL,PLAYER_PLAYER_DEV, &req, sizeof(req), &rep, sizeof(rep)) < 0)
    return -1;

  if (rep.access != access)
  {
    PLAYERC_ERR2("requested [%d] access, but got [%d] access", access, rep.access);
    return -1;
  }

  // Copy the driver name
  strncpy(drivername, rep.driver_name, len);

  return 0;
}


// Unsubscribe from a device
int playerc_client_unsubscribe(playerc_client_t *client, int code, int index)
{
  player_device_req_t req;
  player_device_resp_t rep;

//  req.subtype = htons(PLAYER_PLAYER_DEV_REQ);
  req.code = htons(code);
  req.index = htons(index);
  req.access = PLAYER_CLOSE_MODE;

  if (playerc_client_request(client, NULL, PLAYER_PLAYER_DEV, &req, sizeof(req), &rep, sizeof(rep)) < 0)
    return -1;

  if (rep.access != PLAYER_CLOSE_MODE)
  {
    PLAYERC_ERR2("requested [%d] access, but got [%d] access", PLAYER_CLOSE_MODE, rep.access);
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
int playerc_client_readpacket(playerc_client_t *client, player_msghdr_t *header,
                              char *data, int *len)
{
  int nbytes, bytes, total_bytes;

  // Look for STX
  for (bytes = 0; bytes < 2;)
  {
    nbytes = recv(client->sock, (char*) header + bytes, 2 - bytes, 0);
    if (nbytes <= 0)
    {
      PLAYERC_ERR1("recv on stx failed with error [%s]", strerror(errno));
      return -1;
    }
    bytes += nbytes;
  }
  if (bytes < 2)
  {
    PLAYERC_ERR2("got incomplete stx, %d of %d bytes", bytes, 2);
    return -1;
  }
  if (ntohs(header->stx) != PLAYER_STXX)
  {
    PLAYERC_ERR("malformed packet; discarding");
    return -1;
  }

  // Read packet
  for (bytes = 0; bytes < sizeof(player_msghdr_t) - 2;)
  {
    nbytes = recv(client->sock, (char*) header + 2 + bytes,
                  sizeof(player_msghdr_t) - 2 - bytes, 0);
    if (nbytes <= 0)
    {
      PLAYERC_ERR1("recv on header failed with error [%s]", strerror(errno));
      return -1;
    }
    bytes += nbytes;
  }
  if (bytes < sizeof(player_msghdr_t) - 2)
  {
    PLAYERC_ERR2("got incomplete header, %d of %d bytes", bytes, sizeof(player_msghdr_t) - 2);
    return -1;
  }
  
  // Do the network byte re-ordering 
  header->stx = ntohs(header->stx);
  header->type = (header->type);
  header->subtype = (header->subtype);
  header->device = ntohs(header->device);
  header->device_index = ntohs(header->device_index);
  header->size = ntohl(header->size);
  header->time_sec = ntohl(header->time_sec);
  header->time_usec = ntohl(header->time_usec);
  header->timestamp_sec = ntohl(header->timestamp_sec);
  header->timestamp_usec = ntohl(header->timestamp_usec);

  if (header->size > *len)
  {
    PLAYERC_ERR1("packet is too large, %d bytes", header->size);
    return -1;
  }

  // Read in the body of the packet
  total_bytes = 0;
  while (total_bytes < header->size)
  {
    bytes = recv(client->sock, data + total_bytes, header->size - total_bytes, 0);
    if (bytes <= 0)
    {
      PLAYERC_ERR1("recv on body failed with error [%s]", strerror(errno));
      return -1;
    }
    total_bytes += bytes;
  }

  *len = total_bytes;
  return 0;
}


// Write a raw packet
int playerc_client_writepacket(playerc_client_t *client, player_msghdr_t *header,
                               char *data, int len)
{
  int bytes;

  // Do the network byte re-ordering 
  header->stx = htons(header->stx);
  header->type = (header->type);
  header->subtype = (header->subtype);
  header->device = htons(header->device);
  header->device_index = htons(header->device_index);
  header->size = htonl(header->size);
  
  bytes = send(client->sock, header, sizeof(player_msghdr_t), 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("send on header failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < sizeof(player_msghdr_t))
  {
    PLAYERC_ERR2("sent incomplete header, %d of %d bytes", bytes, sizeof(player_msghdr_t));
    return -1;
  }
  // Now undo the network byte re-ordering 
  header->stx = ntohs(header->stx);
  header->type = (header->type);
  header->subtype = (header->subtype);
  header->device = ntohs(header->device);
  header->device_index = ntohs(header->device_index);
  header->size = ntohl(header->size);
    
  bytes = send(client->sock, data, len, 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("send on body failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < len)
  {
    PLAYERC_ERR2("sent incomplete body, %d of %d bytes", bytes, len);
    return -1;
  }

  return 0;
}


// Push a packet onto the incoming queue.
void playerc_client_push(playerc_client_t *client,
                         player_msghdr_t *header, void *data, int len)
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
  item->data = malloc(len);
  memcpy(item->data, data, len);
  item->len = len;

  client->qlen +=1;

  return;
}


// Pop a packet from the incoming queue.  Returns non-zero if the
// queue is empty.
int playerc_client_pop(playerc_client_t *client,
                       player_msghdr_t *header, void *data, int *len)
{
  playerc_client_item_t *item;

  if (client->qlen == 0)
    return -1;

  item = client->qitems + client->qfirst;
  *header = item->header;
  assert(*len >= item->len);
  memcpy(data, item->data, item->len);
  free(item->data);
  *len = item->len;
  
  client->qfirst = (client->qfirst + 1) % client->qsize;
  client->qlen -= 1;

  return -1;
}


// Dispatch a packet
void *playerc_client_dispatch(playerc_client_t *client, player_msghdr_t *header,
                              void *data, int len)
{
  int i, j;
  playerc_device_t *device;
  
  // Look for a device proxy to handle this data 
  for (i = 0; i < client->device_count; i++)
  {
    device = client->device[i];
        
    if (device->code == header->device && device->index == header->device_index)
    {
      // Fill out timing info 
      device->datatime = header->timestamp_sec + header->timestamp_usec * 1e-6;

      // Call the registerd handler for this device 
      (*device->putdata) (device, (char*) header, data, len);

      // mark as fresh
      device->fresh = 1;

      // Call any additional registered callbacks 
      for (j = 0; j < device->callback_count; j++)
        (*device->callback[j]) (device->callback_data[j]);

      return device->id;
    }
  }
  return NULL;
}

// Dispatch a packet
void *playerc_client_dispatch_geom(playerc_client_t *client, player_msghdr_t *header,
                              void *data, int len)
{
  int i;
  playerc_device_t *device;

  // Look for a device proxy to handle this data 
  for (i = 0; i < client->device_count; i++)
  {
    device = client->device[i];
        
    if (device->code == header->device && device->index == header->device_index)
    {
      // Call the registerd handler for this device 
      if (device->putgeom)
	      (*device->putgeom) (device, (char*) header, data, len);
      else
	    PLAYERC_WARN2("No Geom Message Handler for device %d:%d\n",device->code, device->index);

      // mark as fresh
      device->freshgeom = 1;

      return device->id;
    }
  }
  return NULL;
}

// Dispatch a packet
void *playerc_client_dispatch_config(playerc_client_t *client, player_msghdr_t *header,
                              void *data, int len)
{
  int i;
  playerc_device_t *device;

  // Look for a device proxy to handle this data 
  for (i = 0; i < client->device_count; i++)
  {
    device = client->device[i];
        
    if (device->code == header->device && device->index == header->device_index)
    {
      // Call the registerd handler for this device 
      if (device->putconfig)
	      (*device->putconfig) (device, (char*) header, data, len);

      // mark as fresh
      device->freshconfig = 1;

      return device->id;
    }
  }
  return NULL;
}
