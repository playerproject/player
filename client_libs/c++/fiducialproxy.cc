/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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

/*
 * $Id$
 *
 * client-side beacon device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>

#include "playerpacket.h"

void FiducialProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  player_fiducial_data_t *data = (player_fiducial_data_t*) buffer;
  
  if(hdr.size != sizeof(player_fiducial_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of fiducial data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_fiducial_data_t),hdr.size);
  }

  count = ntohs(data->count);
  memset(beacons,0,sizeof(beacons));
  for(unsigned short i = 0; i < count && i < PLAYER_FIDUCIAL_MAX_SAMPLES; i++)
  {
    beacons[i].id = ntohs(data->fiducials[i].id);
    beacons[i].pose[0] = ((int16_t)ntohs(data->fiducials[i].pose[0])) / 1e3;
    beacons[i].pose[1] = DTOR((double)(int16_t)ntohs(data->fiducials[i].pose[1]));
    beacons[i].pose[2] = DTOR((double)(int16_t)ntohs(data->fiducials[i].pose[2]));
    beacons[i].upose[0] = ((int16_t)ntohs(data->fiducials[i].upose[0])) / 1e3;
    beacons[i].upose[1] = DTOR((double)(int16_t)ntohs(data->fiducials[i].upose[1]));
    beacons[i].upose[2] = DTOR((double)(int16_t)ntohs(data->fiducials[i].upose[2]));
  }
}

// interface that all proxies SHOULD provide
void FiducialProxy::Print()
{
  printf("#Fiducial(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  puts("#count");
  printf("%d\n", count);
  puts("#id\trange\tbear\torient\tr_err\tb_err\to_err");
  for(unsigned short i=0;i<count && i<PLAYER_FIDUCIAL_MAX_SAMPLES;i++)
    printf("%d\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\t%.3f\n", 
	   beacons[i].id, 
	   beacons[i].pose[0], beacons[i].pose[1], beacons[i].pose[2],
	   beacons[i].upose[0], beacons[i].upose[1], beacons[i].upose[2]);
}


int FiducialProxy::PrintFOV()
{
  int res = this->GetFOV();  
  if( res < 0 ) return res;

  printf("#Fiducial(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);
  printf( "#FOV\tmin_range\tmax_range\tview_angle\n"
	  "\t%.2f\t\t%.2f\t\t%.2f\n", 
	  this->min_range, this->max_range, this->view_angle );
  return(res);
}

int FiducialProxy::PrintGeometry()
{
  printf("#Fiducial(%d:%d) - %c\n",
         m_device_id.code, m_device_id.index, access);

  int res = this->GetConfigure();  
  if( res < 0 ) return res;
  
  printf( "#geometry:\n  pose [%.2f %.2f %.2f]  size [%.2f %.2f]"
	  "   target size [%.2f %.2f]\n",
	  pose[0], pose[1], pose[2], size[0], size[1], 
	  fiducial_size[0], fiducial_size[1] );
  
  return res;
}



// Get the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int FiducialProxy::GetConfigure()
{
  int len;
  player_fiducial_geom_t config;
  player_msghdr_t hdr;

  config.subtype = PLAYER_FIDUCIAL_GET_GEOM;

  len = client->Request(m_device_id,
			(const char*)&config, sizeof(config.subtype),
			&hdr, (char*)&config, sizeof(config) );

  if( len == -1 )
    {
      puts( "fiducial config geometry request failed" );
      return(-1);
    }
  
  this->pose[0] = ((int16_t) ntohs(config.pose[0])) / 1e3;
  this->pose[1] = ((int16_t) ntohs(config.pose[1])) / 1e3;
  this->pose[2] = DTOR((double)(int16_t) ntohs(config.pose[2]));
  this->size[0] = ((int16_t) ntohs(config.size[0])) / 1e3;
  this->size[1] = ((int16_t) ntohs(config.size[1])) / 1e3;
  
  this->fiducial_size[0] = 
    ((int16_t) ntohs(config.fiducial_size[0])) / 1000.0;
  this->fiducial_size[1] = 
    ((int16_t) ntohs(config.fiducial_size[1])) / 1000.0;
  
  return 0; // OK
}

// Get the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int FiducialProxy::GetFOV()
{
 
  player_fiducial_fov_t fov;
  player_msghdr_t hdr;

  fov.subtype = PLAYER_FIDUCIAL_GET_FOV;
  
  int result = client->Request(m_device_id,
			       (const char*)&fov, sizeof(fov.subtype),
			       &hdr, (char*)&fov, sizeof(fov) );
  
  if( result < 0 || hdr.type != PLAYER_MSGTYPE_RESP_ACK )
    {
      puts( "fiducial config FOV request failed" );
      return(-1);
    }

  FiducialFovUnpack( &fov, 
		     &this->min_range, &this->max_range, &this->view_angle );

  return 0; // OK
}



// Set the fiducial geometry.  The writes the result into the proxy
// rather than returning it to the caller.
int FiducialProxy::SetFOV( double min_range, 
			   double max_range, 
			   double view_angle)
{
  int len;
  player_fiducial_fov_t fov;
  player_msghdr_t hdr;
  
  FiducialFovPack( &fov, 1, min_range, max_range, view_angle );
    
  len = client->Request(m_device_id,
			(const char*)&fov, sizeof(fov),
			&hdr, (char*)&fov, sizeof(fov) );
  
  if( len < 0 || hdr.type != PLAYER_MSGTYPE_RESP_ACK )
    {
      puts( "fiducial config FOV request failed" );
      return(-1);
    }

  FiducialFovUnpack( &fov, 
		     &this->min_range, &this->max_range, &this->view_angle );
  
  return 0; // OK
}

/*
  Attempt to send a message to a fiducial. See the Player manual
  for details of the message packet. Use a target_id of -1 to
  broadcast. Note: these message functions use configs that are
  probably only supported by Stage-1.4 (or later) fiducial
  driver.
*/

int FiducialProxy::SendMessage( player_fiducial_msg_t* msg, bool consume )
{
  // build the transmit request
  player_fiducial_msg_tx_req_t tx_req;
  tx_req.subtype = PLAYER_FIDUCIAL_SEND_MSG;
  tx_req.consume = (uint8_t)consume;

  // copy the message into the send struct, byteswapping fields
  tx_req.msg.target_id = (int32_t)htonl(msg->target_id);
  tx_req.msg.intensity = (uint8_t)msg->intensity;
  memcpy( tx_req.msg.bytes, msg->bytes, PLAYER_FIDUCIAL_MAX_MSG_LEN );
  tx_req.msg.len = (uint8_t)msg->len;
  
  //printf( "sending message of %d bytes\n", msg->len );
  
  player_msghdr_t hdr;
  if( client->Request(m_device_id,
		      (const char*)&tx_req, sizeof(tx_req),
		      &hdr, NULL, 0 ) < 0 )
    {
      puts( "error: fiducial send message request failed" );
      return(-1);
    }
  
  if( hdr.type != PLAYER_MSGTYPE_RESP_ACK )
    {
    puts( "error: fiducial send message request received NACK" );
    return(-1);
  }

  return 0; // OK
}

/* Read a message received by the device. If a message is available,
   the recv_msg packet is filled in and 0 is returned.  no message can
   be retrieved from the device, returns -1. If consume is true, the
   message is deleted from the device on reading. If false, the
   message is kept and can be read again. Note: these message
   functions use configs that are probably only supported by Stage-1.4
   (or later) fiducial driver.
*/

int FiducialProxy::RecvMessage( player_fiducial_msg_t* recv_msg, 
				bool consume )
{ 
  assert(recv_msg);
  
  player_fiducial_msg_rx_req_t rx_req;
  rx_req.subtype = PLAYER_FIDUCIAL_RECV_MSG;
  rx_req.consume = (uint8_t)consume;
  
  //printf( "requesting receive message\n" );
  
  player_msghdr_t hdr;
  if( client->Request(m_device_id,
		      (const char*)&rx_req, 
		      sizeof(player_fiducial_msg_rx_req_t),
		      &hdr, (char*)recv_msg,  
		      sizeof(player_fiducial_msg_t)) < 0 )
     {
       puts( "error: fiducial recv message request failed" );
       return(-1);
     }
  
  // byteswap the fields for local use
  recv_msg->target_id = ntohl( recv_msg->target_id );

  // Stage replies with NACK if the request failed, for example if no
  // message was available
  if(  hdr.type != PLAYER_MSGTYPE_RESP_ACK )
      return(-1);
  
  return 0; // OK
}
