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

#define PLAYER_ENABLE_TRACE 1

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
}


///////////////////////////////////////////////////////////////////////////
// Read a message from the incoming queue
int BroadcastProxy::Read(void *msg, int len)
{
  // TODO
  return -1;
}

 
///////////////////////////////////////////////////////////////////////////
// Write a message to the outgoing queue
int BroadcastProxy::Write(void *msg, int len)
{
  // TODO
  return -1;
}


///////////////////////////////////////////////////////////////////////////
// Update the incoming queue (does nothing)
void BroadcastProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
}


///////////////////////////////////////////////////////////////////////////
// Debugging function (does nothing)
void BroadcastProxy::Print()
{
}



