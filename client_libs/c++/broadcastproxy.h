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
 * client-side broadcast device 
 */

#ifndef BROADCASTPROXY_H
#define BROADCASTPROXY_H

#include <clientproxy.h>
#include <playerclient.h>

/** The {\tt BroadcastProxy} class controls the {\tt broadcast} device.
    Data may be read one message at a time from the incoming broadcast
    queue using the {\tt Read} method.
    Data may be written one message at a time to the outgoing broadcast
    queue using the {\tt Write} method.
    Note that outgoing messages are not actually sent to the server
    until the {\tt Flush} method is called.
 */
class BroadcastProxy : public ClientProxy
{
  /** Proxy constructor.
      Leave the access field empty to start unconnected.
      You can change the access later using {\tt PlayerProxy::RequestDeviceAccess}.
  */
  public: BroadcastProxy(PlayerClient* pc, unsigned short index, unsigned char access ='c');

  /** Read a message from the incoming queue.
      Returns the number of bytes read, or -1 if the queue is empty.
  */
  public: int Read(void *msg, int len);

  /** Write a message to the outgoing queue.
      Returns the number of bytes written, or -1 if the queue is full.
  */
  public: int Write(void *msg, int len);

  // interface that all proxies must provide
  protected: void FillData(player_msghdr_t hdr, const char* buffer);
    
  // interface that all proxies SHOULD provide
  protected: void Print();
};

#endif
