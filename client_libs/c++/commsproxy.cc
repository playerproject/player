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
#include <stdlib.h>


///////////////////////////////////////////////////////////////////////////
// Constructor
CommsProxy::CommsProxy(PlayerClient* pc, unsigned short index, unsigned char access)
    : ClientProxy(pc, PLAYER_COMMS_CODE, index, access)
{
  this->listlen = 10;
  this->msg = (uint8_t**)calloc(this->listlen,sizeof(uint8_t*));
  this->msg_len = (size_t*)calloc(this->listlen,sizeof(size_t));
  this->msg_ts = (struct timeval*)calloc(this->listlen,sizeof(struct timeval));
  this->msg_num = 0;
  return;
}

///////////////////////////////////////////////////////////////////////////
// Destructor
CommsProxy::~CommsProxy()
{
  for(size_t i=0;i<this->msg_num;i++)
    free(this->msg[i]);
  free(this->msg);
  free(this->msg_len);
  free(this->msg_ts);
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
  
  // might have to resize the list
  if(this->listlen == this->msg_num)
  {
    this->listlen *= 2;
    this->msg = (uint8_t**)realloc(this->msg,this->listlen * sizeof(uint8_t*));
    this->msg_len = (size_t*)realloc(this->msg_len,this->listlen * sizeof(size_t));
    this->msg_ts = (struct timeval*)realloc(this->msg_ts,this->listlen * sizeof(struct timeval));
  }

  this->msg[this->msg_num] = (uint8_t*)malloc(hdr.size);
  memcpy(this->msg[this->msg_num], buffer, hdr.size);
  this->msg_len[this->msg_num] = hdr.size;
  this->msg_ts[this->msg_num].tv_sec = hdr.timestamp_sec;
  this->msg_ts[this->msg_num].tv_usec = hdr.timestamp_usec;
  this->msg_num++;

  return;
}

///////////////////////////////////////////////////////////////////////////
// Delete a message (and shift the rest down)
int CommsProxy::Delete(int index)
{
  if((index < 0) || (index >= (int)this->msg_num))
    return(-1);

  free(this->msg[index]);
  for(size_t i=index;i<this->msg_num-1;i++)
  {
    this->msg[i] = this->msg[i+1];
    this->msg_len[i] = this->msg_len[i+1];
    this->msg_ts[i] = this->msg_ts[i+1];
  }
  this->msg_num--;
  return(0);
}




///////////////////////////////////////////////////////////////////////////
// Debugging function (does nothing)
void CommsProxy::Print()
{
  printf("# Comms(%d:%d) - %c : %d messages\n", device, index, access,
         this->msg_num);
  for(size_t i=0;i<this->msg_num;i++)
  {
    printf("# len %d msg [%s]\n", this->msg_len[i], this->msg[i]);
    printf("# timestamp: %ld:%ld\n", 
           this->msg_ts[i].tv_sec, this->msg_ts[i].tv_usec);
  }
  return;
}
