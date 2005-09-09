/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
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

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <libplayerxdr/playerxdr.h>
#include "remote_driver.h"

TCPRemoteDriver::TCPRemoteDriver(player_devaddr_t addr, void* arg) 
        : Driver(NULL, 0, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  if(arg)
    this->ptcp = (PlayerTCP*)arg;
  else
    this->ptcp = NULL;

  this->sock = -1;
}

TCPRemoteDriver::~TCPRemoteDriver()
{
}

int 
TCPRemoteDriver::Setup() 
{
  struct sockaddr_in server;
  char banner[PLAYER_IDENT_STRLEN];

  packedaddr_to_dottedip(this->ipaddr,sizeof(this->ipaddr),
                         this->device_addr.host);

  printf("trying to Setup %s:%d:%d:%d\n",
         this->ipaddr,
         this->device_addr.robot,
         this->device_addr.interf,
         this->device_addr.index);

  // Construct socket 
  this->sock = socket(PF_INET, SOCK_STREAM, 0);
  if(this->sock < 0)
  {
    PLAYER_ERROR1("socket call failed with error [%s]", strerror(errno));
    return(-1);
  }

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = this->device_addr.host;
  server.sin_port = htons(this->device_addr.robot);

  // Connect the socket 
  if(connect(this->sock, (struct sockaddr*)&server, sizeof(server)) < 0)
  {
    PLAYER_ERROR3("connect call on [%s:%u] failed with error [%s]",
                this->ipaddr,
                this->device_addr.robot, 
                strerror(errno));
    return(-1);
  }

  printf("connected to: %s:%u\n",
         this->ipaddr, this->device_addr.robot);

  // Get the banner 
  if(read(this->sock, banner, sizeof(banner)) < (int)sizeof(banner))
  {
    PLAYER_ERROR("incomplete initialization string");
    return(-1);
  }
  if(this->SubscribeRemote() < 0)
  {
    close(this->sock);
    return(-1);
  }

  // make the socket non-blocking
  if(fcntl(this->sock, F_SETFL, O_NONBLOCK) == -1)
  {
    PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
    close(this->sock);
    return(-1);
  }

  // Add this socket for monitoring
  this->queue = this->ptcp->AddClient(NULL, this->device_addr.robot, 
                                      this->sock, false);

  return(0);
}

// subscribe to the remote device
int
TCPRemoteDriver::SubscribeRemote()
{
  int encode_msglen;
  unsigned char buf[512];
  player_msghdr_t hdr;
  player_device_req_t req;

  memset(&hdr,0,sizeof(hdr));
  hdr.addr.interf = PLAYER_PLAYER_CODE;
  hdr.type = PLAYER_MSGTYPE_REQ;
  hdr.subtype = PLAYER_PLAYER_REQ_DEV;
  GlobalTime->GetTimeDouble(&hdr.timestamp);

  req.addr = this->device_addr;
  req.access = PLAYER_OPEN_MODE;
  req.driver_name_count = 0;

  // Encode the body
  if((encode_msglen = 
      player_device_req_pack(buf + PLAYERXDR_MSGHDR_SIZE,
                             sizeof(buf)-PLAYERXDR_MSGHDR_SIZE,
                             &req, PLAYERXDR_ENCODE)) < 0)
  {
    PLAYER_ERROR("failed to encode request");
    return(-1);
  }

  // Rewrite the size in the header with the length of the encoded
  // body, then encode the header.
  hdr.size = encode_msglen;
  if(player_msghdr_pack(buf, PLAYERXDR_MSGHDR_SIZE, &hdr,
                        PLAYERXDR_ENCODE) < 0)
  {
    PLAYER_ERROR("failed to encode header");
    return(-1);
  }

  encode_msglen += PLAYERXDR_MSGHDR_SIZE;

  // Send the request
  if(write(this->sock, buf, encode_msglen) < encode_msglen)
  {
    PLAYER_ERROR1("write failed: %s", strerror(errno));
    return(-1);
  }
  
  // Receive the response
  if((encode_msglen = read(this->sock, buf, sizeof(buf))) < 0)
  {
    PLAYER_ERROR1("read failed: %s", strerror(errno));
    return(-1);
  }

  // Decode the header
  if(player_msghdr_pack(buf, encode_msglen, &hdr, PLAYERXDR_DECODE) < 0)
  {
    PLAYER_ERROR("failed to decode header");
    return(-1);
  }

  // Is it the right kind of message?
  if(!Message::MatchMessage(&hdr, 
                            PLAYER_MSGTYPE_RESP_ACK,
                            PLAYER_PLAYER_REQ_DEV,
                            hdr.addr))
  {
    PLAYER_ERROR("got wrong kind of reply");
    return(-1);
  }

  memset(&req,0,sizeof(req));
  // Decode the body
  if(player_device_req_pack(buf + PLAYERXDR_MSGHDR_SIZE,
                            encode_msglen-PLAYERXDR_MSGHDR_SIZE,
                            &req, PLAYERXDR_DECODE) < 0)
  {
    PLAYER_ERROR("failed to decode reply");
    return(-1);
  }

  // Did we get the right access?
  if(req.access != PLAYER_OPEN_MODE)
  {
    PLAYER_ERROR("got wrong access");
    return(-1);
  }

  // Success!
  PLAYER_MSG5(1, "Subscribed to %s:%d:%d:%d (%s)",
              this->ipaddr,
              this->device_addr.robot,
              this->device_addr.interf,
              this->device_addr.index,
              req.driver_name);

  return(0);
}

int 
TCPRemoteDriver::Shutdown() 
{ 
  printf("trying to Shutdown %s:%d:%d:%d\n",
         this->ipaddr,
         this->device_addr.robot,
         this->device_addr.interf,
         this->device_addr.index);
  
  if(this->sock >= 0)
    close(this->sock);
  this->sock = -1;
  return(0); 
}

void 
TCPRemoteDriver::Update()
{
  if(this->ptcp->thread == pthread_self())
    this->ptcp->Read(0);
  this->ProcessMessages();
  if(this->ptcp->thread == pthread_self())
    this->ptcp->Write();
}

int 
TCPRemoteDriver::ProcessMessage(MessageQueue* resp_queue, 
                                player_msghdr * hdr, 
                                void * data)
{
  // Is it data from the remote device?
  if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_DATA,
                           -1,
                           this->device_addr))
  {
    // Re-publish it for local consumers
    this->Publish(NULL, hdr, data);
    return(0);
  }
  // Is it a command for the remote device?
  else if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_CMD,
                           -1,
                           this->device_addr))
  {
    // Push it onto the outgoing queue
    this->Publish(this->queue, hdr, data);
    return(0);
  }
  // Is it a request for the remote device?
  else if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_REQ,
                           -1,
                           this->device_addr))
  {
    // Push it onto the outgoing queue
    this->Publish(this->queue, hdr, data);
    // Store the return address for later use
    this->ret_queue = resp_queue;
    // Set the message filter to look for the response
    this->InQueue->SetFilter(this->device_addr.host,
                             this->device_addr.robot,
                             this->device_addr.interf,
                             this->device_addr.index,
                             -1,
                             hdr->subtype);
    // No response now; it will come later after we hear back from the
    // laser
    return(0);
  }
  // Forward response (success or failure) from the laser
  else if((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, 
                                 -1, this->device_addr)) ||
          (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK,
                                 -1, this->device_addr)))
  {
    // Forward the response
    this->Publish(this->ret_queue, hdr, data);
    // Clear the filter
    this->InQueue->ClearFilter();

    return(0);
  }
  else
    return(-1);
}


Driver* 
TCPRemoteDriver::TCPRemoteDriver_Init(player_devaddr_t addr, void* arg)
{
  return((Driver*)(new TCPRemoteDriver(addr, arg)));
}
