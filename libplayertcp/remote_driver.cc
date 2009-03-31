/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) <insert dates here>
 *     <insert author's name(s) here>
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
/********************************************************************
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
 *
 ********************************************************************/

#include <config.h>

#if !defined (WIN32)
  #include <unistd.h>
#endif
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>

#if ENABLE_TCP_NODELAY
  #include <netinet/tcp.h>
#endif

#include <libplayercore/globals.h>
#include <libplayercore/error.h>
#include <libplayerxdr/playerxdr.h>
#include "remote_driver.h"
#include "playertcp_errutils.h"

#if defined (WIN32)
  #define write _write
  #define read _read
#endif

TCPRemoteDriver::TCPRemoteDriver(player_devaddr_t addr, void* arg)
        : Driver(NULL, 0, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN),
          sock(-1),
          setup_timeout (DEFAULT_SETUP_TIMEOUT)
{
    ptcp = reinterpret_cast<PlayerTCP*> (arg);
}

TCPRemoteDriver::~TCPRemoteDriver()
{
}

int
TCPRemoteDriver::Setup()
{
  struct sockaddr_in server;
  char banner[PLAYER_IDENT_STRLEN];
  int numread, thisnumread;
  double t1, t2;

  packedaddr_to_dottedip(this->ipaddr,sizeof(this->ipaddr),
                         this->device_addr.host);

  // We can't talk to ourselves
  if(this->ptcp->GetHost() == this->device_addr.host && this->ptcp->Listening(this->device_addr.robot))
  {
    PLAYER_ERROR4("tried to connect to self (%s:%d:%d:%d)\n",
                  this->ipaddr,
                  this->device_addr.robot,
                  this->device_addr.interf,
                  this->device_addr.index);
    return(-1);
  }


  // Construct socket
  this->sock = socket(PF_INET, SOCK_STREAM, 0);
  if(this->sock < 0)
  {
    PLAYER_ERROR1("socket call failed with error [%s]", strerror(errno));
    return(-1);
  }

  server.sin_family = AF_INET;
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

  PLAYER_MSG2(2,"connected to: %s:%u\n",
         this->ipaddr, this->device_addr.robot);

  // make the socket non-blocking
#if defined (WIN32)
  unsigned long setting = 1;
  if (ioctlsocket(sock, FIONBIO, &setting) == SOCKET_ERROR)
  {
    STRERROR(PLAYER_ERROR1, "ioctlsocket error: %s");
    closesocket(sock);
    return(-1);
  }
#else
  if(fcntl(this->sock, F_SETFL, O_NONBLOCK) == -1)
  {
    PLAYER_ERROR1("fcntl() failed: %s", strerror(errno));
    close(this->sock);
    return(-1);
  }
#endif

#if ENABLE_TCP_NODELAY
  // Disable Nagel's algorithm for lower latency
  int yes = 1;
  if( setsockopt(this->sock, IPPROTO_TCP, TCP_NODELAY, &yes,
                 sizeof(int)) == -1 )
  {
    PLAYER_ERROR("failed to enable TCP_NODELAY - setsockopt failed");
    return(-1);
  }
#endif

  // Get the banner.
  GlobalTime->GetTimeDouble(&t1);
  numread=0;
  while(numread < (int)sizeof(banner))
  {
    thisnumread = recv(this->sock, banner+numread, sizeof(banner)-numread, 0);
    if(thisnumread < 0)
    {
      if(ErrNo != ERRNO_EAGAIN)
      {
        // Error on the socket
#if defined (WIN32)
        LPVOID buffer = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                      ErrNo, 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
        PLAYER_MSG1(2, "recv() failed: %s", reinterpret_cast<LPTSTR> (buffer));
        LocalFree(buffer);
#else
        PLAYER_MSG1(2,"recv() failed: %s", strerror(ErrNo));
#endif
        PLAYER_ERROR("error reading banner from remote device");
        return(-1);
      }
    }
    else if(thisnumread == 0)
    {
      PLAYER_ERROR("error reading banner from remote device (connection closed by peer)");
      return(-1);
    }
    else
      numread += thisnumread;
    GlobalTime->GetTimeDouble(&t2);
    if((t2-t1) > this->setup_timeout)
    {
      PLAYER_ERROR("timed out reading banner from remote server");
      return(-1);
    }
  }
  if(this->SubscribeRemote(PLAYER_OPEN_MODE) < 0)
  {
#if defined (WIN32)
    closesocket(this->sock);
#else
    close(this->sock);
#endif
    return(-1);
  }

  PLAYER_MSG0(5,"Adding new TCPRemoteDriver to the PlayerTCP Client List");

  // Add this socket for monitoring
  this->kill_flag = 0;
  this->queue = this->ptcp->AddClient(NULL,
                                      this->device_addr.host,
                                      this->device_addr.robot,
                                      this->sock,
                                      false,
                                      &this->kill_flag,
									  pthread_equal (this->ptcp->thread, pthread_self()) != 0 ? true : false);

  PLAYER_MSG0(5,"Adding new TCPRemoteDriver to the PlayerTCP Client List...Success");

  return(0);
}

// (un)subscribe to the remote device
int
TCPRemoteDriver::SubscribeRemote(unsigned char mode)
{
  int encode_msglen;
  unsigned char buf[512];
  player_msghdr_t hdr;
  player_device_req_t req;
  double t1, t2;

  int numbytes, thisnumbytes;

  memset(&hdr,0,sizeof(hdr));
  hdr.addr.interf = PLAYER_PLAYER_CODE;
  hdr.type = PLAYER_MSGTYPE_REQ;
  hdr.subtype = PLAYER_PLAYER_REQ_DEV;
  GlobalTime->GetTimeDouble(&hdr.timestamp);

  req.addr = this->device_addr;
  req.access = mode;
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
  GlobalTime->GetTimeDouble(&t1);
  numbytes = 0;

  while(numbytes < encode_msglen)
  {
    if((thisnumbytes = write(this->sock, buf+numbytes,
                             encode_msglen-numbytes)) < 0)
    {
      if(errno != ERRNO_EAGAIN)
      {
        PLAYER_ERROR1("write failed: %s", strerror(errno));
        return(-1);
      }
    }
    else
    {
      numbytes += thisnumbytes;
    }
    GlobalTime->GetTimeDouble(&t2);
    if((t2-t1) > this->setup_timeout)
    {
      PLAYER_ERROR("timed out sending subscription request to remote server");
      return(-1);
    }
  }

  // If we're closing, then stop here; we don't need to hear the response.
  // In any case, explicitly unsubscribing is just a courtesy.
  if(mode == PLAYER_CLOSE_MODE)
    return(0);

  // Receive the response header
  GlobalTime->GetTimeDouble(&t1);
  numbytes = 0;
  while(numbytes < PLAYERXDR_MSGHDR_SIZE)
  {
    thisnumbytes = recv(this->sock, buf+numbytes, PLAYERXDR_MSGHDR_SIZE-numbytes, 0);
    if(thisnumbytes < 0)
    {
      if(ErrNo != ERRNO_EAGAIN)
      {
        // Error on the socket
#if defined (WIN32)
        LPVOID buffer = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                      ErrNo, 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
        PLAYER_MSG1(2, "recv() failed: %s", reinterpret_cast<LPTSTR> (buffer));
        LocalFree(buffer);
#else
        PLAYER_MSG1(2, "recv() failed: %s", strerror(ErrNo));
#endif
        PLAYER_ERROR("error reading message header from remote device");
        return(-1);
      }
    }
    else if(thisnumbytes == 0)
    {
      PLAYER_ERROR("error reading message header from remote device (connection closed by peer)");
      return(-1);
    }
    else
      numbytes += thisnumbytes;
    GlobalTime->GetTimeDouble(&t2);
    if((t2-t1) > this->setup_timeout)
    {
      PLAYER_ERROR("timed out reading response header from remote server");
      return(-1);
    }
  }

  // Decode the header
  if(player_msghdr_pack(buf, PLAYERXDR_MSGHDR_SIZE, &hdr, PLAYERXDR_DECODE) < 0)
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

  // Receive the response body
  GlobalTime->GetTimeDouble(&t1);
  numbytes = 0;
  while(numbytes < (int)hdr.size)
  {
    thisnumbytes = recv(this->sock, buf+PLAYERXDR_MSGHDR_SIZE+numbytes, hdr.size-numbytes, 0);
    if(thisnumbytes < 0)
    {
      if(ErrNo != ERRNO_EAGAIN)
      {
        // Error on the socket
#if defined (WIN32)
        LPVOID buffer = NULL;
        FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
                      ErrNo, 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
        PLAYER_MSG1(2, "recv() failed: %s", reinterpret_cast<LPTSTR> (buffer));
        LocalFree(buffer);
#else
        PLAYER_MSG1(2, "recv() failed: %s", strerror(ErrNo));
#endif
        PLAYER_ERROR("error reading message body from remote device");
        return(-1);
      }
    }
    else if(thisnumbytes == 0)
    {
      PLAYER_ERROR("error reading message body from remote device (connection closed by peer)");
      return(-1);
    }
    else
      numbytes += thisnumbytes;
    GlobalTime->GetTimeDouble(&t2);
    if((t2-t1) > this->setup_timeout)
    {
      PLAYER_ERROR("timed out reading response body from remote server");
      return(-1);
    }
  }

  memset(&req,0,sizeof(req));
  // Decode the body
  if(player_device_req_pack(buf + PLAYERXDR_MSGHDR_SIZE,
                            hdr.size, &req, PLAYERXDR_DECODE) < 0)
  {
    PLAYER_ERROR("failed to decode reply");
    return(-1);
  }

  // Did we get the right access?
  if(req.access != mode)
  {
    PLAYER_ERROR("got wrong access");
    return(-1);
  }

  // Success!
  PLAYER_MSG5(1, "(un)subscribed to/from remote device %s:%d:%d:%d (%s)",
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
  // Have we already been killed?
  if(!this->kill_flag)
  {
    // Unsubscribe
    if(this->SubscribeRemote(PLAYER_CLOSE_MODE) < 0)
      PLAYER_WARN("failed to unsubscribe from remote device");

    // Set the delete flag, letting PlayerTCP close the connection and
    // clean up.
    this->ptcp->DeleteClient(this->queue,
                             pthread_equal(this->ptcp->thread, pthread_self()) != 0 ? true : false);
  }
  return(0);
}

void 
TCPRemoteDriver::Update()
{
  if (pthread_equal(this->ptcp->thread, pthread_self()))
  {
    //this->ptcp->Read(0,true);
    this->ptcp->Lock();
    this->ptcp->ReadClient(this->queue);
    this->ptcp->Unlock();
  }
  this->ProcessMessages();
  if (pthread_equal(this->ptcp->thread, pthread_self()))
    this->ptcp->Write(false);
}

int
TCPRemoteDriver::ProcessMessage(QueuePointer &resp_queue,
                                player_msghdr * hdr,
                                void * data)
{
  // Is it data from the remote device?
  if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_DATA,
                           -1,
                           this->device_addr))
  {
    // Re-publish the message for local consumers
    this->Publish(hdr, data);
    return(0);
  }
  // Is it a command for the remote device?
  else if(Message::MatchMessage(hdr,
                           PLAYER_MSGTYPE_CMD,
                           -1,
                           this->device_addr))
  {
    // Has our connection to the remote device been closed?
    if(this->kill_flag)
      return(0);

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
    // Has our connection to the remote device been closed?
    if(this->kill_flag)
    {
      // Send a NACK
      return(-1);
    }
    else
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
