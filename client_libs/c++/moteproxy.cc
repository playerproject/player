/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Gabe Sibley & Brian Gerkey
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Gabe Sibley & Brian Gerkey
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

#define PLAYER_ENABLE_TRACE 0

#include "playerclient.h"
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

///////////////////////////////////////////////////////////////////////////
// Constructor
MoteProxy::MoteProxy(PlayerClient* pc, unsigned short index, 
                     unsigned char access)
        : ClientProxy(pc, PLAYER_MOTE_CODE, index, access)
{
  memset(&this->m_config, 0, sizeof(this->m_config));
  rx_queue = (player_mote_data_t*)malloc(sizeof(player_mote_data_t)
					*MAX_MOTE_Q_LEN); 
  rx_data = NULL;
  r_flag = TRUE;
  msg_q_index = 0;
}

///////////////////////////////////////////////////////////////////////////
// Write a message to mote radio
int MoteProxy::TransmitRaw(char *msg, uint16_t len)
{
  player_mote_data_t tx_data;

  if(!client)
    return(-1);

  tx_data.len = len;
  memcpy(tx_data.buf, msg, len);
  
  return(client->Write(m_device_id,(const char*)&tx_data,
                         sizeof(player_mote_data_t)));
}

///////////////////////////////////////////////////////////////////////////
int MoteProxy::SetStrength(uint8_t str)
{
  if(!client)
    return(-1);
  
  m_config.strength = str;
  
  return(client->Request(m_device_id,(const char*)&m_config,
			 sizeof(player_mote_config_t)));
}

///////////////////////////////////////////////////////////////////////////
int MoteProxy::GetStrength(void)
{
  return m_config.strength; 
}


///////////////////////////////////////////////////////////////////////////
// for consistent feeling interface
int MoteProxy::RecieveRaw(char* msg, uint16_t len, double *rssi)
{
  if(!client){
    return(-1);
  }
  if(rx_data){
    len = rx_data->len;
    memcpy(msg, rx_data->buf, len);
    *rssi = rx_data->rssi;
    rx_data++;
    msg_q_index++;
    if(msg_q_index == MAX_MOTE_Q_LEN || rx_data->len == 0){
      rx_data = NULL;
    }
    return len;
  }
  return 0;
}



///////////////////////////////////////////////////////////////////////////
inline float MoteProxy::GetRSSI(void)
{
  if(!client)
    return(-1);
  return rx_data->rssi;
}

///////////////////////////////////////////////////////////////////////////
// Update the incoming queue
void MoteProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  
  memcpy(rx_queue, buffer, MAX_MOTE_Q_LEN * sizeof(player_mote_data_t));
  rx_data = rx_queue;
  msg_q_index = 0;
}


///////////////////////////////////////////////////////////////////////////
// Debugging function
void MoteProxy::Print()
{
  if(!client)
    return;
  rx_data->buf[rx_data->len] = 0;
  printf("%s, len %d\n", 
	 rx_data->buf, rx_data->len);
}















