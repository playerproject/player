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
 * client-side position device 
 */

#define PLAYER_ENABLE_TRACE 0

#include <broadcastproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif


///////////////////////////////////////////////////////////////////////////
// Constructor
BroadcastProxy::BroadcastProxy(PlayerClient* pc, unsigned short index, unsigned char access)
        : ClientProxy(pc, PLAYER_BROADCAST_CODE, index, access)
{
    memset(&this->cmd, 0, sizeof(this->cmd));
    memset(&this->data, 0, sizeof(this->data));
}


///////////////////////////////////////////////////////////////////////////
// Read a message from the incoming queue
// Returns the number of bytes read
// Returns -1 if there are no available messages
int BroadcastProxy::Read(uint8_t *msg, uint16_t len)
{
    if (this->data.len <= 0)
        return -1; 

    // Get the next message in the queue
    uint16_t _len = ntohs(*((uint16_t*) this->data.buffer));
    uint8_t *_msg = this->data.buffer + sizeof(_len);

    PLAYER_TRACE1("read queue %d bytes", this->data.len);
    
    // Make copy of message
    assert(len >= _len);
    memcpy(msg, _msg, _len);

    PLAYER_TRACE2("read msg [%s] from queue %d bytes", _msg, _len);

    // Now move everything in the queue down
    memmove(this->data.buffer, this->data.buffer + _len + sizeof(_len),
            this->data.len - _len - sizeof(_len));
    this->data.len -= _len + sizeof(_len);

    PLAYER_TRACE0("done moving queue");

    return _len;
}

 
///////////////////////////////////////////////////////////////////////////
// Write a message to the outgoing queue
// Returns the number of bytes written
// Returns -1 if the queue is full
int BroadcastProxy::Write(uint8_t *msg, uint16_t len)
{
    // Check for overflow
    if (this->cmd.len + len + sizeof(len) > sizeof(this->cmd.buffer))
        return -1;

    PLAYER_TRACE2("wrote msg [%s] to queue %d bytes", msg, len);
    
    uint16_t xlen = htons(len);
    memcpy(this->cmd.buffer + this->cmd.len, &xlen, sizeof(len));
    memcpy(this->cmd.buffer + this->cmd.len + sizeof(len), msg, len);
    this->cmd.len += len + sizeof(len);

    return len;
}


///////////////////////////////////////////////////////////////////////////
// Flush the outgoing message queue
int BroadcastProxy::Flush()
{
    PLAYER_TRACE1("wrote %d bytes", this->cmd.len);
    
    // Do some byte swapping
    this->cmd.len = htons(this->cmd.len);

    // Write our command
    if (this->client->Write(PLAYER_BROADCAST_CODE, this->index,
                            (char*) &this->cmd, ntohs(this->cmd.len) + sizeof(this->cmd.len)) < 0)
        return -1;

    // Reset the command buffer
    this->cmd.len = 0;

    return 0;
}


///////////////////////////////////////////////////////////////////////////
// Update the incoming queue
void BroadcastProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
    this->data.len = ntohs(((player_broadcast_data_t*) buffer)->len);
    PLAYER_TRACE1("fill %d bytes", this->data.len);
    memcpy(this->data.buffer, buffer + sizeof(this->data.len), this->data.len);
}


///////////////////////////////////////////////////////////////////////////
// Debugging function
void BroadcastProxy::Print()
{
}



