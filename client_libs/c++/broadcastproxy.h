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

class BroadcastProxy : public ClientProxy
{

    // Constructor
    public: BroadcastProxy(PlayerClient* pc, unsigned short index,
                           unsigned char access ='c');

    // Read a message from the incoming queue
    // Returns -1 if there are no available messages
    public: int Read(uint8_t *msg, uint16_t *len);

    // Write a message to the outgoing queue
    // Returns -1 if the queue is full
    public: int Write(uint8_t *msg, uint16_t len);

    // Flush the outgoing message queue
    public: int Flush();

    // interface that all proxies must provide
    protected: void FillData(player_msghdr_t hdr, const char* buffer);
    
    // interface that all proxies SHOULD provide
    protected: void Print();
    
    // Queue of incoming messages
    private: player_broadcast_data_t data;

    // Queue of outgoing messages
    private: player_broadcast_cmd_t cmd;
};

#endif
