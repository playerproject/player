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
        : ClientProxy(pc, PLAYER_MOTE_CODE, index, access)
{
  memset(&this->m_config, 0, sizeof(this->m_config));
  rx_data = (player_mote_data_t*) malloc(sizeof(player_mote_data_t));
  tx_data = (player_mote_data_t*) malloc(sizeof(player_mote_data_t));
  new_data = 0;
}

///////////////////////////////////////////////////////////////////////////
// Write a message to mote radio
// Returns the number of bytes written
// Returns -1 if the queue is full
int MoteProxy::TransmitRaw(char *msg, uint16_t len)
{
  if(!client)
    return(-1);

  tx_data->len = len;
  tx_data->src = 0;
  tx_data->dest = 1;
  memcpy(tx_data->buf, msg, len);

  return(client->Write(PLAYER_MOTE_CODE,index,(const char*)tx_data,
                         sizeof(player_mote_data_t)));
}

///////////////////////////////////////////////////////////////////////////
int MoteProxy::SetStrength(uint8_t str)
{
  if(!client)
    return(-1);
  
  m_config.strength = str;
  
  return(client->Request(PLAYER_MOTE_CODE,index,(const char*)&m_config,
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

  if(new_data){
    new_data = FALSE;
    memcpy(msg, rx_data->buf, len);
    *rssi = rx_data->rssi;
    return rx_data->len;
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
  if( (((player_mote_data_t*)buffer)->len != 0)){
    new_data = TRUE;
    memcpy(rx_data, buffer, sizeof(player_mote_data_t));
  }
}


///////////////////////////////////////////////////////////////////////////
// Debugging function
void MoteProxy::Print()
{
  if(!client)
    return;
  rx_data->buf[rx_data->len] = 0;
  printf("%s, src %d, dest %d, len %d\n", 
	 rx_data->buf, rx_data->src, rx_data->dest, rx_data->len);
}















