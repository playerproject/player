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

#include "remote_driver.h"

TCPRemoteDriver::TCPRemoteDriver(player_devaddr_t addr, void* arg) 
        : Driver(NULL, 0, false, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  if(this->ptcp)
    this->ptcp = (PlayerTCP*)arg;
  else
    this->ptcp = NULL;
}

TCPRemoteDriver::~TCPRemoteDriver()
{
}

int 
TCPRemoteDriver::Setup() 
{ 
  struct sockaddr_in server;
  int sock;
  char banner[PLAYER_IDENT_STRLEN];

  printf("trying to Setup %d:%d:%d:%d\n",
         this->device_addr.host,
         this->device_addr.robot,
         this->device_addr.interf,
         this->device_addr.index);

  server.sin_family = PF_INET;
  server.sin_addr = this->device_addr.host;
  server.sin_port = htons(this->device_addr.port);

  // Connect the socket 
  if(connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0)
  {
    PLAYER_ERR3("connect call on [%d:%d] failed with error [%s]",
                this->device_addr.host, 
                this->device_addr.port, 
                strerror(errno));
    return(-1);
  }

  // Get the banner 
  if(recv(client->sock, banner, sizeof(banner), 0) < sizeof(banner))
  {
    PLAYER_ERR("incomplete initialization string");
    return(-1);
  }

  // TODO: subscribe to the remote device, and record it here somewhere.

  this->AddClient(NULL, 0, sock, this);

  return(0); 
}

int 
TCPRemoteDriver::Shutdown() 
{ 
  printf("trying to Shutdown %d:%d:%d:%d\n",
         this->device_addr.host,
         this->device_addr.robot,
         this->device_addr.interf,
         this->device_addr.index);
  return(-1); 
}

void 
TCPRemoteDriver::Update() 
{
}

Driver* 
TCPRemoteDriver::TCPRemoteDriver_Init(player_devaddr_t addr, void* arg)
{
  printf("TCPRemoteDriver_Init(%d:%d:%d:%d)\n",
         addr.host, addr.robot, addr.interf, addr.index);
  return((Driver*)(new TCPRemoteDriver(addr, arg)));
}
