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
 */

//#define PLAYER_ENABLE_TRACE 1

#include <playerclient.h>

#include <netinet/in.h>
#include <string.h>


///////////////////////////////////////////////////////////////////////////
// Constructor
CommsProxy::CommsProxy(PlayerClient* pc, unsigned short index, unsigned char access)
    : ClientProxy(pc, PLAYER_COMMS_CODE, index, access)
{
  this->msg_len = 0;
  return;
}

 
///////////////////////////////////////////////////////////////////////////
// Write a message to the outgoing queue
int CommsProxy::Write(void *msg, int len)
{
  if (len > PLAYER_MAX_MESSAGE_SIZE)
  {
    fprintf(stderr, "outgoing message too long; %d > %d bytes.",
            len, PLAYER_MAX_MESSAGE_SIZE);
    return -1;
  }
  return client->Write(PLAYER_COMMS_CODE, index, (const char *) msg, len);
}



///////////////////////////////////////////////////////////////////////////
// Update the incoming queue 
void CommsProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  if (hdr.size > PLAYER_MAX_MESSAGE_SIZE)
  {
    fprintf(stderr, "incoming message too long; %d > %d bytes.",
            hdr.size, PLAYER_MAX_MESSAGE_SIZE);
    return;
  }
  
  this->msg_len = hdr.size;
  memcpy(this->msg, buffer, hdr.size);

  return;
}


///////////////////////////////////////////////////////////////////////////
// Debugging function (does nothing)
void CommsProxy::Print()
{
  printf("# Comms(%d:%d) - %c\n", device, index, access);
  printf("# len %d msg [%s]\n", this->msg_len, this->msg);
  return;
}



