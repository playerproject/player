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
 * the core functions for a pure C client for Player
 */

#include "playercclient.h"
#include <netdb.h>
#include <unistd.h>
#include <string.h> /* for bzero() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

/*
 * use this to turn off debug ouput.
 *
 * higher numbers are more output, 0 is none.
 *
 * incidentally, it returns the current level, and if you give 
 * -1 for <level>, then the current level is unchanged
 */
int player_debug_level(int level)
{
  static int debug_level = PLAYER_CCLIENT_DEBUG_LEVEL_DEFAULT;

  if(level >= 0)
    debug_level = level;

  return(debug_level);
}

/*
 * connects to server listening at host:port.  conn is filled in with
 * relevant information, and is used in subsequent player function
 * calls
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened)
 */
int player_connect(player_connection_t* conn, const char* host, int port)
{
  struct hostent* entp;
  struct sockaddr_in server;

  /* pointers must be good */
  assert( conn );
  assert( host );
  
  /* 
   * this is okay to do, because gethostbyname(3) does no lookup if the 
   * 'host' * arg is already an IP addr
   */
  if((entp = gethostbyname(host)) == NULL)
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, "player_connect() \"%s\" is an unknown host\n", host);
    return(-1);
  }

  /* bounds check before copying in the address */
  assert(sizeof(server.sin_addr) >= entp->h_length);
  memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);

  /* fill in server structure */
  server.sin_family = PF_INET;
  server.sin_port = htons(port);
  
  /* make the connection */
  return player_connect_sockaddr( conn, &server );
}

/*
 * connects to server listening at host:port.  conn is filled in with
 * relevant information, and is used in subsequent player function
 * calls
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened)
 */
int player_connect_ip(player_connection_t* conn, 
		   const struct in_addr* addr, 
		   const int port)
{
  struct sockaddr_in server;

  /* pointers must be good */
  assert( conn );
  assert( addr );

  /* fill in server structure */
  server.sin_family = PF_INET;
  server.sin_port = htons(port);

  /* bounds check before copying in the address */
  assert(sizeof(server.sin_addr) <= sizeof(addr->s_addr));
  memcpy(&server.sin_addr, &addr->s_addr, sizeof(addr->s_addr) );
  
  /* make the connection */
  return player_connect_sockaddr( conn, &server );
}

/*
 * connects to server listening at the address specified in sockaddr.
 * conn is filled in with relevant information, and is used in
 * subsequent player function calls
 *
 * Returns:
 *    0 if everything is OK (connection opened)
 *   -1 if something went wrong (connection NOT opened) */
int player_connect_sockaddr(player_connection_t* conn, 
			    const struct sockaddr_in* server)
{
  int sock;
  char banner[PLAYER_IDENT_STRLEN];
  int thisnumread,numread;

  /* pointers must be good */
  assert(conn);
  assert(server);

  /* make our socket (and leave it blocking) */
  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    if(player_debug_level(-1) >= 2)
      perror("player_connect(): socket() failed");
    return(-1);
  }

  /* 
   * hook it up
   */
  if(connect(sock, (struct sockaddr*)server, sizeof(*server)) == -1)
  {
    if(player_debug_level(-1) >= 2)
      perror("player_connect(): connect() failed");
    close(sock);
    return(-1);
  }

  /*
   * read the banner from the server
   */
  numread = 0;
  while(numread < PLAYER_IDENT_STRLEN)
  {
    if((thisnumread = 
        read(sock,banner+numread,PLAYER_IDENT_STRLEN-numread)) < 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_connect(): read() failed");
      close(sock);
      return(-1);
    }

    numread += thisnumread;
  }

  /* fill in the caller's structure */
  (*conn).sock = sock;
  memcpy((*conn).banner,banner,PLAYER_IDENT_STRLEN);

  return(0);
}

/*
 * close a connection. conn should be a value that was previously returned
 * by a call to player_connect()
 *
 * Returns:
 *    0 if everything is OK (connection closed)
 *   -1 if something went wrong (connection not closed)
 */
int player_disconnect(player_connection_t* conn)
{
  if(close((*conn).sock) == -1)
  {
    if(player_debug_level(-1) >= 2)
      perror("player_disconnect(): close() failed");
    return(-1);
  }
  (*conn).sock = -1;
  return(0);
}

/*
 * issue some request to the server. payloadlen is the length of the 
 * payload.  reply, if non-NULL, will be used to hold the reply; replylen
 * is the size of the buffer (player_request() will not overrun your buffer)
 *
 *   Returns:
 *      0 if everything's OK
 *     -1 if something went wrong (you should probably close the connection!)
 */
int player_request(player_connection_t* conn, 
                   uint16_t device, uint16_t device_index, 
                   const char* payload, size_t payloadlen, 
                   player_msghdr_t* replyhdr, char* reply, size_t replylen)
{
  unsigned char buffer[PLAYER_MAX_MESSAGE_SIZE];
  player_msghdr_t hdr;

  if(payloadlen > (PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t)))
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, "player_request(): tried to send too large of a payload"
              "(%d bytes > %d bytes); message NOT sent.\n", 
              payloadlen, 
              (PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t)));
    return(-1);
  }
  hdr.stx = htons(PLAYER_STXX);
  hdr.type = htons(PLAYER_MSGTYPE_REQ);
  hdr.device = htons(device);
  hdr.device_index = htons(device_index);
  hdr.time_sec = 0;
  hdr.time_usec = 0;
  hdr.timestamp_sec = 0;
  hdr.timestamp_usec = 0;
  hdr.reserved = 0;
  hdr.size = htonl(payloadlen);

  memcpy(buffer,&hdr,sizeof(player_msghdr_t));
  memcpy(buffer+sizeof(player_msghdr_t),payload,payloadlen);

  if(conn->sock < 0)
    return(-1);

  /* write the request */
  if(write((*conn).sock,buffer, sizeof(player_msghdr_t)+payloadlen) == -1)
  {
    if(player_debug_level(-1) >= 2)
      perror("player_request(): write() failed.");
    return(-1);
  }

  bzero(&hdr,sizeof(hdr));
  /* eat data until a response comes back (it may not be for the request we
   * just sent, but let a higher level deal with that problem) */
  while((hdr.type != PLAYER_MSGTYPE_RESP_ACK) &&
         (hdr.type != PLAYER_MSGTYPE_RESP_NACK) &&
         (hdr.type != PLAYER_MSGTYPE_RESP_ERR))
  {
    if(player_read(conn, &hdr, buffer, sizeof(buffer)) == -1)
      return(-1);
  }

  /* did they want the reply? */
  if(replyhdr)
    *replyhdr = hdr;
  if(reply && replylen >= hdr.size)
    memcpy(reply,buffer,replyhdr->size);

  return(0);
}

/*
 * issue a single device request
 */
int player_request_device_access(player_connection_t* conn,
                                 uint16_t device,
                                 uint16_t device_index,
                                 uint8_t req_access,
                                 uint8_t* grant_access )
{
  player_device_req_t this_req;
  unsigned char payload[sizeof(player_device_req_t)];
  player_msghdr_t replyhdr;
  unsigned char replybuffer[PLAYER_MAX_MESSAGE_SIZE];

  this_req.subtype = htons(PLAYER_PLAYER_DEV_REQ);
  this_req.code = htons(device);
  this_req.index = htons(device_index);
  this_req.access = req_access;

  memcpy(payload,&this_req,sizeof(player_device_req_t));

  if(player_request(conn, PLAYER_PLAYER_CODE, 0, 
                    payload, sizeof(payload),
                    &replyhdr, replybuffer, sizeof(replybuffer)) == -1)
    return(-1);

  memcpy(&this_req, replybuffer, sizeof(player_device_req_t));

  if(grant_access)
    *grant_access = this_req.access;
  else if(memcmp(payload,replybuffer,sizeof(payload)))
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, "player_request_device_access(): requested '%c' access "
              "to device %x:%x, but got '%c' access\n",
              req_access, device, device_index, this_req.access);
  }

  return(0);
}

/*
 * read one message header from the indicated connection. 
 *
 * Returns:
 *    0 if everything went OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_read_header(player_connection_t* conn, player_msghdr_t* hdr )
{
  int readcnt=0,thisreadcnt;

  if(conn->sock < 0)
    return(-1);

  hdr->stx = 0;
  /* wait for the STX */
  while(ntohs(hdr->stx) != PLAYER_STXX)
  {
    if((readcnt = read((*conn).sock,&(hdr->stx),sizeof(hdr->stx))) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while looking for STX");
      return(-1);
    }
  }

  /* get the rest of the header */
  while(readcnt < sizeof(player_msghdr_t))
  {
    if((thisreadcnt = read((*conn).sock, ((char*)hdr)+readcnt,
                      sizeof(player_msghdr_t)-readcnt)) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while reading header.");
      return(-1);
    }
    readcnt += thisreadcnt;
  }

  /* byte-swap as necessary */
  hdr->type = ntohs(hdr->type);
  hdr->device = ntohs(hdr->device);
  hdr->device_index = ntohs(hdr->device_index);
  hdr->time_sec = ntohl(hdr->time_sec);
  hdr->time_usec = ntohl(hdr->time_usec);
  hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
  hdr->timestamp_usec = ntohl(hdr->timestamp_usec);
  hdr->size = ntohl(hdr->size);
  /*printf("time: %Lu\tts:%Lu\n", hdr->time,hdr->timestamp);*/
  /*timesec = (time_t)(hdr->time / 1000);*/
  /*printf("time: %s\n", ctime(&timesec));*/
  
  return 0;
}

/*
 * read the data part of a message from the indicated connection.  put the 
 * data in buffer, up to bufferlen.
 *
 * Returns:
 *    0 if everything went OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_read_payload(player_connection_t* conn, char* payload, size_t payloadlen)
{
  int readcnt=0,thisreadcnt;

  if(conn->sock < 0)
    return(-1);

  readcnt = 0;
  while(readcnt < payloadlen)
  {
    if((thisreadcnt = read((*conn).sock,payload+readcnt,payloadlen-readcnt)) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while reading payload.");
      return(-1);
    }
    readcnt += thisreadcnt;
  }

  return(0);
}

/*
 * read one complete message from the indicated connection.  put the 
 * data in buffer, up to bufferlen.
 *
 * Returns:
 *    0 if everything went OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_read(player_connection_t* conn, player_msghdr_t* hdr,
                char* payload, size_t payloadlen)
{
  /*time_t timesec;*/
  int mincnt; 
  int readcnt=0,thisreadcnt;
  char dummy[PLAYER_MAX_MESSAGE_SIZE];

  if(conn->sock < 0)
    return(-1);

  hdr->stx = 0;
  /* wait for the STX */
  while(ntohs(hdr->stx) != PLAYER_STXX)
  {
    if((readcnt = read((*conn).sock,&(hdr->stx),sizeof(hdr->stx))) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while looking for STX");
      return(-1);
    }
  }

  /* get the rest of the header */
  while(readcnt < sizeof(player_msghdr_t))
  {
    if((thisreadcnt = read((*conn).sock, ((char*)hdr)+readcnt,
                      sizeof(player_msghdr_t)-readcnt)) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while reading header.");
      return(-1);
    }
    readcnt += thisreadcnt;
  }

  /* byte-swap as necessary */
  hdr->type = ntohs(hdr->type);
  hdr->device = ntohs(hdr->device);
  hdr->device_index = ntohs(hdr->device_index);
  hdr->time_sec = ntohl(hdr->time_sec);
  hdr->time_usec = ntohl(hdr->time_usec);
  hdr->timestamp_sec = ntohl(hdr->timestamp_sec);
  hdr->timestamp_usec = ntohl(hdr->timestamp_usec);
  hdr->size = ntohl(hdr->size);
  /*printf("time: %Lu\tts:%Lu\n", hdr->time,hdr->timestamp);*/
  /*timesec = (time_t)(hdr->time / 1000);*/
  /*printf("time: %s\n", ctime(&timesec));*/
  
  /* get the payload */
  if(hdr->size > payloadlen)
    if(player_debug_level(-1) >= 2)
      fprintf(stderr,"WARNING: server's message is too big (%d bytes). "
              "Truncating data.\n", hdr->size);

  mincnt = min(hdr->size, payloadlen);

  readcnt = 0;
  while(readcnt < mincnt)
  {
    if((thisreadcnt = read((*conn).sock,payload+readcnt,mincnt-readcnt)) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while reading payload.");
      return(-1);
    }
    readcnt += thisreadcnt;
  }

  while(readcnt < hdr->size)
  {
    if((thisreadcnt = read((*conn).sock,dummy,
                           min(PLAYER_MAX_MESSAGE_SIZE,hdr->size-readcnt))) <= 0)
    {
      if(player_debug_level(-1) >= 2)
        perror("player_read(): read() errored while reading excess bytes.");
      return(-1);
    }
    readcnt += thisreadcnt;
  }

  return(0);
}

/*
 * write commands to the indicated connection. writes the data contained
 * in command, up to commandlen.
 *
 * Returns:
 *    0 if everything goes OK
 *   -1 if something went wrong (you should probably close the connection!)
 */
int player_write(player_connection_t* conn, 
                 uint16_t device, uint16_t device_index,
                 const char* command, size_t commandlen)
{
  return(_player_write(conn,device,device_index,command,commandlen,0));
}

int _player_write(player_connection_t* conn, 
                 uint16_t device, uint16_t device_index,
                 const char* command, size_t commandlen, int reserved)
{
  char buffer[PLAYER_MAX_MESSAGE_SIZE];
  player_msghdr_t hdr;

  if(commandlen > PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t))
  {
    if(player_debug_level(-1) >= 2)
      fprintf(stderr, "player_write(): tried to send too large of a command"
              "(%d bytes > %d bytes); message NOT sent.\n", 
              commandlen, 
              (PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t)));
    return(-1);
  }

  hdr.stx = htons(PLAYER_STXX);
  hdr.type = htons(PLAYER_MSGTYPE_CMD);
  hdr.device = htons(device);
  hdr.device_index = htons(device_index);
  hdr.time_sec = 0;
  hdr.time_usec = 0;
  hdr.timestamp_sec = 0;
  hdr.timestamp_usec = 0;
  hdr.reserved = htonl(reserved);
  hdr.size = htonl(commandlen);
  memcpy(buffer,&hdr,sizeof(player_msghdr_t));
  memcpy(buffer+sizeof(player_msghdr_t),command,commandlen);

  if(conn->sock < 0)
    return(-1);

  if(write((*conn).sock, buffer, sizeof(player_msghdr_t)+commandlen) !=
                  sizeof(player_msghdr_t)+commandlen)
  {
    if(player_debug_level(-1) >= 2)
      perror("player_write(): write() errored.");
    return(-1);
  }

  return(0);
}

