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
}


///////////////////////////////////////////////////////////////////////////
// Read a message from the incoming queue
int CommsProxy::Read(void *msg, int len)
{
  player_comms_msg_t req, rep;
  player_msghdr_t hdr;
  int reqlen;

  req.subtype = PLAYER_COMMS_SUBTYPE_RECV;
  reqlen = sizeof(req.subtype);

  if(client->Request(PLAYER_COMMS_CODE,index,
                     (const char*)&req, reqlen,
                     &hdr, (char*)&rep,sizeof(rep)) ||
     
     hdr.type != PLAYER_MSGTYPE_RESP_ACK)
    return(-1);

  if((int)hdr.size > len)
  {
    printf("WARNING: received comms msg too long (%d > %d)\n "
           "truncating msg\n", hdr.size, len);
    hdr.size = len;
  }

  memcpy(msg,rep.data,hdr.size);

  return(hdr.size);
}

 
///////////////////////////////////////////////////////////////////////////
// Write a message to the outgoing queue
int CommsProxy::Write(void *msg, int len)
{
  player_comms_msg_t req;
  int reqlen, replen;

  if(len > (int)sizeof(req.data))
  {
    printf("WARNING: sent comms msg too long (%d > %d);\n "
           "truncating msg\n", len, sizeof(req.data));
    len = sizeof(req.data);
  }

  req.subtype = PLAYER_COMMS_SUBTYPE_SEND;
  memcpy(req.data,msg,len);
  reqlen = sizeof(req) - sizeof(req.data) + len;

  replen = client->Request(PLAYER_COMMS_CODE,index,
                           (const char*)&req, reqlen);

  if(replen < 0)
    return(replen);

  return(0);
}



///////////////////////////////////////////////////////////////////////////
// Update the incoming queue (does nothing)
void CommsProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
}


///////////////////////////////////////////////////////////////////////////
// Debugging function (does nothing)
void CommsProxy::Print()
{
}



