#define PLAYER_ENABLE_TRACE 0

#include "playerclient.h"
#include <moteproxy.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#ifdef PLAYER_SOLARIS 
#include <strings.h>
#endif

///////////////////////////////////////////////////////////////////////////
// Constructor
MoteProxy::MoteProxy(PlayerClient* pc, unsigned short index, unsigned char access)
        : ClientProxy(pc, PLAYER_MOTE_TYPE, index, access)
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
  
  return(client->Write(PLAYER_MOTE_TYPE,index,(const char*)&tx_data,
                         sizeof(player_mote_data_t)));
}

///////////////////////////////////////////////////////////////////////////
int MoteProxy::SetStrength(uint8_t str)
{
  if(!client)
    return(-1);
  
  m_config.strength = str;
  
  return(client->Request(PLAYER_MOTE_TYPE,index,(const char*)&m_config,
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















