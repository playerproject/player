/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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
 */
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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
 */

/* $Id$
 *
 *  MComProxy class by Matt Brewer at UMass Amherst 2002
 *  added to 1.3 by reed
 */

#include <stdio.h>
#include <playerclient.h>
    
int MComProxy::Push(int type, char * channelQ,char* dat){
  if(!client)
    return(-1);
  player_mcom_config_t cfg;
  //cfg.command = PLAYER_MCOM_PUSH_REQ;
  cfg.type=htons(type);
  strcpy(cfg.channel,channelQ);
  cfg.data.full=true;
  memcpy(cfg.data.data,dat,MCOM_DATA_LEN);
  int r = client->Request(m_device_id, PLAYER_MCOM_PUSH,(const char*)&cfg, sizeof(cfg));
  if(r < 0) {
      printf("mcomproxy: error (%d) sending request\n", r);
      return r;
  }
  return 0;
}

int MComProxy::Read(int type, char * channelQ){
    player_msghdr_t hdr;
    if(!client) 
        return(-1);
    player_mcom_config_t cfg;
//    cfg.command = PLAYER_MCOM_READ_REQ;
    cfg.type=htons(type);
    strcpy(cfg.channel,channelQ);
    player_mcom_return_t reply;
    int r = client->Request(m_device_id, PLAYER_MCOM_READ,
            (const char*)&cfg, sizeof(cfg), &hdr, 
            (char*)&reply, sizeof(reply));
    if(r < 0)
        return r;
    if(hdr.type != PLAYER_MSGTYPE_RESP_ACK) {
        memset(&data, 0, sizeof(data));
        type = 0;
        memset(channel, 0, sizeof(channel));
        return -1;
    }
    data=reply.data;
    type=htons(reply.type);
    strcpy(channel,reply.channel);
    return 0;
}

int MComProxy::Pop(int type, char* channelQ){
  player_msghdr_t hdr;
  if(!client)
    return(-1);

  player_mcom_config_t cfg;
//  cfg.command = PLAYER_MCOM_POP_REQ;
  cfg.type=htons(type);
  strcpy(cfg.channel,channelQ);
  player_mcom_return_t reply;
  int r = client->Request(m_device_id, PLAYER_MCOM_POP,
		      (const char*)&cfg, sizeof(cfg), &hdr , 
              (char*)&reply, sizeof(reply));
  if(r < 0)
    return r; 
  if(hdr.type != PLAYER_MSGTYPE_RESP_ACK) {
    memset(&data, 0, sizeof(data));
    type = 0;
    memset(channel, 0, sizeof(channel));
    return(-1);
  }
  data=reply.data;
  type=htons(reply.type);
  strcpy(channel,reply.channel);
  return 0;
}

int MComProxy::Clear(int type, char * channelQ){
  if(!client)
    return(-1);
  
  player_mcom_config_t cfg;
//  cfg.command = PLAYER_MCOM_CLEAR_REQ;
  cfg.type=htons(type);
  strcpy(cfg.channel,channelQ);
  return client->Request(m_device_id, PLAYER_MCOM_CLEAR, 
			 (const char*)&cfg,sizeof(cfg));
}

int
MComProxy::SetCapacity(int type, char *channel, unsigned char cap) 
{
  if (!client) {
    return -1;
  }

  player_mcom_config_t cfg;

//  cfg.command = PLAYER_MCOM_SET_CAPACITY_REQ;
  cfg.type = htons(type);
  strncpy(cfg.channel, channel, MCOM_CHANNEL_LEN);
  cfg.data.data[0] = cap;

  return client->Request(m_device_id, PLAYER_MCOM_SET_CAPACITY, (const char *)&cfg, sizeof(cfg));
}





void MComProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
}

void MComProxy::Print()
{
    // TODO: do something here
}





