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
 * a pure C client
 */

#include <playercclient.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h> /* for bzero() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

/*#include <time.h>*/  /* temporary */

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
  struct sockaddr_in server;
  struct hostent* entp;
  int sock;
  char banner[PLAYER_IDENT_STRLEN];
  int numread;

  /* fill in server structure */
  server.sin_family = PF_INET;

  /* 
   * this is okay to do, because gethostbyname(3) does no lookup if the 
   * 'host' * arg is already an IP addr
   */
  if((entp = gethostbyname(host)) == NULL)
  {
    fprintf(stderr, "player_connect() \"%s\" is an unknown host\n", host);
    return(-1);
  }

  memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);
  server.sin_port = htons(port);

  /* make our socket (and leave it blocking) */
  if((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    perror("player_connect(): socket() failed");
    return(-1);
  }

  /* 
   * hook it up
   */
  if(connect(sock, (struct sockaddr*)&server, sizeof(server)) == -1)
  {
    perror("player_connect(): connect() failed");
    close(sock);
    return(-1);
  }

  /*
   * read the banner from the server
   */
  if((numread = read(sock,banner,PLAYER_IDENT_STRLEN)) != PLAYER_IDENT_STRLEN)
  {
    perror("player_connect(): read() failed");
    close(sock);
    return(-1);
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
 *      0 if everything went OK
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
    fprintf(stderr, "player_request(): tried to send too large of a payload"
                    "(%d bytes > %d bytes); message NOT sent.\n", 
                    payloadlen, 
                    (PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t)));
    return(-1);
  }
  hdr.stx = PLAYER_STX;
  hdr.type = PLAYER_MSGTYPE_REQ;
  hdr.device = device;
  hdr.device_index = htons(device_index);
  hdr.time = 0;
  hdr.timestamp = 0;
  hdr.reserved = 0;
  hdr.size = htonl(payloadlen);

  memcpy(buffer,&hdr,sizeof(player_msghdr_t));
  memcpy(buffer+sizeof(player_msghdr_t),payload,payloadlen);

  /* write the request */
  if(write((*conn).sock,buffer, sizeof(player_msghdr_t)+payloadlen) == -1)
  {
    perror("player_request(): write() failed.");
    return(-1);
  }

  bzero(replyhdr,sizeof(player_msghdr_t));
  /* eat data until the response comes back */
  while((replyhdr->type != PLAYER_MSGTYPE_RESP) || 
        (replyhdr->device != device) ||
        (replyhdr->device_index != device_index))
  {
    if(player_read(conn, replyhdr, reply, replylen) == -1)
      return(-1);
  }

  return(0);
}

/*
 * issue a single device request
 */
int player_request_device_access(player_connection_t* conn,
                                 uint16_t device,
                                 uint16_t device_index,
                                 uint8_t access)
{
  player_device_ioctl_t this_ioctl;
  player_device_req_t this_req;
  unsigned char 
      payload[sizeof(player_device_ioctl_t)+sizeof(player_device_req_t)];
  player_msghdr_t replyhdr;
  unsigned char replybuffer[PLAYER_MAX_MESSAGE_SIZE];

  this_ioctl.subtype = PLAYER_PLAYER_DEV_REQ;
  this_req.code = device;
  this_req.index = device_index;
  this_req.access = access;

  memcpy(payload,&this_ioctl,sizeof(player_device_ioctl_t));
  memcpy(payload+sizeof(player_device_ioctl_t),
                  &this_req,sizeof(player_device_req_t));

  if(player_request(conn, PLAYER_PLAYER_CODE, 0, 
                    payload, sizeof(payload),
                    &replyhdr, replybuffer, sizeof(replybuffer)) == -1)
    return(-1);

  memcpy(&this_ioctl, replybuffer, sizeof(player_device_ioctl_t));
  memcpy(&this_req, replybuffer+sizeof(player_device_ioctl_t), 
                  sizeof(player_device_req_t));

  /* see if something went wrong */
  if(memcmp(payload,replybuffer,sizeof(payload)))
  {
    fprintf(stderr, "player_request_device_access(): requested '%c' access "
                    "to device %x:%x, but got '%c' access\n",
                    access, device, device_index, this_req.access);
    return(-1);
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
  int readcnt;
  char dummy[PLAYER_MAX_MESSAGE_SIZE];

  hdr->stx = 0;
  /* wait for the STX */
  while(hdr->stx != PLAYER_STX)
  {
    if((readcnt = read((*conn).sock,&(hdr->stx),sizeof(hdr->stx))) <= 0)
    {
      perror("player_read(): read() errored while looking for STX");
      return(-1);
    }
  }
  /*puts("got STX");*/

  /* get the rest of the header */
  if((readcnt += read((*conn).sock, &(hdr->type),
                      sizeof(player_msghdr_t)-sizeof(hdr->stx))) != 
                  sizeof(player_msghdr_t))
  {
    perror("player_read(): read() errored while reading header.");
    return(-1);
  }

  /* byte-swap as necessary */
  hdr->device_index = ntohs(hdr->device_index);
  hdr->time = ntohll(hdr->time);
  hdr->timestamp = ntohll(hdr->timestamp);
  hdr->size = ntohl(hdr->size);
  /*printf("time: %Lu\tts:%Lu\n", hdr->time,hdr->timestamp);*/
  /*timesec = (time_t)(hdr->time / 1000);*/
  /*printf("time: %s\n", ctime(&timesec));*/
  /*puts("got HDR");*/

  /* get the payload */
  if(hdr->size > payloadlen)
    fprintf(stderr,"WARNING: server's message is too big (%d bytes). "
                    "Truncating data.", hdr->size);

  mincnt = min(hdr->size, payloadlen);

  for(readcnt  = read((*conn).sock,payload,mincnt);
      readcnt != mincnt;
      readcnt += read((*conn).sock,payload+readcnt,mincnt-readcnt));

  if (readcnt < hdr->size)
  {
      for(readcnt += read((*conn).sock,dummy,hdr->size-readcnt);
          readcnt != hdr->size;
          readcnt += read((*conn).sock,dummy,hdr->size-readcnt));
  }

  //if((readcnt = read((*conn).sock,payload,min(hdr->size,payloadlen)))
                  //!= min(hdr->size,payloadlen))
  //{
    //fprintf(stderr, "client_reader: tried to read server-specified %d bytes,"
                    //"but only got %d\n", hdr->size, readcnt);
    //return(-1);
  //}
  /*puts("got payload");*/
  return(0);
}

/*
 * read laser data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_laser(player_connection_t* conn, player_laser_data_t* data)
{
  player_msghdr_t hdr;
  int i;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_laser_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_LASER_CODE)
  {
    fprintf(stderr, "player_read_laser(): received wrong device code\n");
    return(-1);
  }

  for(i=0;i<PLAYER_NUM_LASER_SAMPLES;i++)
    data->ranges[i] = ntohs(data->ranges[i]);

  return(0);
}

/*
 * read sonar data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_sonar(player_connection_t* conn, player_sonar_data_t* data)
{
  player_msghdr_t hdr;
  int i;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_sonar_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_SONAR_CODE)
  {
    fprintf(stderr, "player_read_sonar(): received wrong device code\n");
    return(-1);
  }

  for(i=0;i<PLAYER_NUM_SONAR_SAMPLES;i++)
    data->ranges[i] = ntohs(data->ranges[i]);

  return(0);
}

/*
 * read position data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_position(player_connection_t* conn,player_position_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_position_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_POSITION_CODE)
  {
    fprintf(stderr, "player_read_position(): received wrong device code\n");
    return(-1);
  }

  data->xpos = ntohl(data->xpos);
  data->ypos = ntohl(data->ypos);
  data->theta = ntohs(data->theta);
  data->speed = ntohs(data->speed);
  data->turnrate = ntohs(data->turnrate);
  data->compass = ntohs(data->compass);

  return(0);
}

/*
 * read misc data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_misc(player_connection_t* conn, player_misc_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_misc_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_MISC_CODE)
  {
    fprintf(stderr, "player_read_misc(): received wrong device code\n");
    return(-1);
  }

  /* no byte-swapping here... */

  return(0);
}

/*
 * read ptz data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_ptz(player_connection_t* conn, player_ptz_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_ptz_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_PTZ_CODE)
  {
    fprintf(stderr, "player_read_ptz(): received wrong device code\n");
    return(-1);
  }

  data->pan = ntohs(data->pan);
  data->tilt = ntohs(data->tilt);
  data->zoom = ntohs(data->zoom);

  return(0);
}

/*
 * read vision data into designated buffer.
 *
 * Returns:
 *   0 if OK
 *  -1 if something wrong (like got unexpected device code)
 */
int player_read_vision(player_connection_t* conn, player_vision_data_t* data)
{
  player_msghdr_t hdr;

  if(player_read(conn, &hdr, (char*)data, sizeof(player_vision_data_t)) == -1)
    return(-1);

  if(hdr.device != PLAYER_VISION_CODE)
  {
    fprintf(stderr, "player_read_vision(): received wrong device code\n");
    return(-1);
  }

  return(0);
}

void player_print_vision(player_vision_data_t data)
{
  int i,j,k;
  int area;
  int bufptr;
  char* buf = (char*)&data;

  bufptr = ACTS_HEADER_SIZE;
  for(i=0;i<ACTS_NUM_CHANNELS;i++)
  {
    if(!(buf[2*i+1]-1))
    {
      /* no blobs on this channel */
      /*printf("no blobs on channel %d\n", i);*/
    }
    else
    {
      printf("%d blob(s) on channel %d\n", buf[2*i+1]-1, i);
      for(j=0;j<buf[2*i+1]-1;j++)
      {
        /* first compute the area */
        area=0;
        for(k=0;k<4;k++)
        {
          area = area << 6;
          area |= buf[bufptr + k] - 1;
        }
        printf("  area:%d\n", area);
        printf("  x:%d\n", (unsigned char)(buf[bufptr + 4] - 1));
        printf("  y:%d\n", (unsigned char)(buf[bufptr + 5] - 1));
        printf("  left:%d\n", (unsigned char)(buf[bufptr + 6] - 1));
        printf("  right:%d\n", (unsigned char)(buf[bufptr + 7] - 1));
        printf("  top:%d\n", (unsigned char)(buf[bufptr + 8] - 1));
        printf("  bottom:%d\n", (unsigned char)(buf[bufptr + 9] - 1));

        bufptr += ACTS_BLOB_SIZE;
      }
    }
  }
}

void player_print_misc(player_misc_data_t data)
{
  int i;
  printf("FrontBumpers: ");
  for(i=0;i<5;i++)
    printf("%d ",(data.frontbumpers) >> i & 0x1);
  printf("\nRearBumpers: ");
  for(i=0;i<5;i++)
    printf("%d ",(data.rearbumpers) >> i & 0x1);
  printf("\nVoltage: %f\n", data.voltage/10.0);
}

void player_print_ptz(player_ptz_data_t data)
{
  printf("pan:%d\ttilt:%d\tzoom:%d\n", data.pan,data.tilt,data.zoom);
}

void player_print_laser(player_laser_data_t data)
{
  int i;
  for(i=0;i<PLAYER_NUM_LASER_SAMPLES;i++)
    printf("laser(%d) = %d\n", i, data.ranges[i]);
}
void player_print_sonar(player_sonar_data_t data)
{
  int i;
  for(i=0;i<PLAYER_NUM_SONAR_SAMPLES;i++)
    printf("sonar(%d): %d\n", i, data.ranges[i]);
}
void player_print_position(player_position_data_t data)
{
  printf("pos: (%d,%d,%d)\n", data.xpos,data.ypos,data.theta);
  printf("speed: %d  turnrate: %d\n", data.speed, data.turnrate);
  printf("compass: %d\n", data.compass);
  printf("stalls: %d\n", data.stalls);
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
                 char* command, size_t commandlen)
{
  char buffer[PLAYER_MAX_MESSAGE_SIZE];
  player_msghdr_t hdr;

  if(commandlen > PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t))
  {
    fprintf(stderr, "player_write(): tried to send too large of a command"
                    "(%d bytes > %d bytes); message NOT sent.\n", 
                    commandlen, 
                    (PLAYER_MAX_MESSAGE_SIZE - sizeof(player_msghdr_t)));
    return(-1);
  }

  hdr.stx = PLAYER_STX;
  hdr.type = PLAYER_MSGTYPE_CMD;
  hdr.device = device;
  hdr.device_index = htons(device_index);
  hdr.time = 0;
  hdr.timestamp = 0;
  hdr.reserved = 0;
  hdr.size = htonl(commandlen);
  memcpy(buffer,&hdr,sizeof(player_msghdr_t));
  memcpy(buffer+sizeof(player_msghdr_t),command,commandlen);

  if(write((*conn).sock, buffer, sizeof(player_msghdr_t)+commandlen) !=
                  sizeof(player_msghdr_t)+commandlen)
  {
    perror("player_write(): write() errored.");
    return(-1);
  }

  return(0);
}

/*
 * write to 0th position device
 */
int player_write_position(player_connection_t* conn, player_position_cmd_t cmd)
{
  player_position_cmd_t swapped_cmd;
  swapped_cmd.speed = htons(cmd.speed);
  swapped_cmd.turnrate = htons(cmd.turnrate);
  return(player_write(conn, PLAYER_POSITION_CODE, 0, 
                          (char*)&swapped_cmd, sizeof(player_position_cmd_t)));
}

/*
 * write to 0th ptz device
 */
int player_write_ptz(player_connection_t* conn, player_ptz_cmd_t cmd)
{
  player_ptz_cmd_t swapped_cmd;
  swapped_cmd.pan = htons(cmd.pan);
  swapped_cmd.tilt = htons(cmd.tilt);
  swapped_cmd.zoom = htons(cmd.zoom);
  return(player_write(conn, PLAYER_PTZ_CODE, 0, 
                          (char*)&swapped_cmd, sizeof(player_ptz_cmd_t)));
}

