/*
 *  Stage-1.4 driver for Player
 *  Copyright (C) 2003  Richard Vaughan (rtv) vaughan@hrl.com 
 *
 * This program is free software; you can redistribute it and/or modify
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
 * $Id$
 */

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"
#include <stdlib.h>
#include "stage1p4.h"

// DRIVER FOR FIDUCIAL INTERFACE ////////////////////////////////////////////////////////

class StgFiducial:public Stage1p4
{
public:
  StgFiducial(char* interface, ConfigFile* cf, int section );

  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual int PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len);
};

StgFiducial::StgFiducial(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_fiducial_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgFiducial with interface %s", interface );
}

CDevice* StgFiducial_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_FIDUCIAL_STRING))
    {
      PLAYER_ERROR1("driver \"stg_fiducial_neighbors\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgFiducial(interface, cf, section)));
}


void StgFiducial_Register(DriverTable* table)
{
  table->AddDriver("stg_fiducial_neighbors", PLAYER_ALL_MODE, StgFiducial_Init);
}

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgFiducial::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  PLAYER_MSG2(" STG_FIDUCIAL GETDATA section %d -> model %d",
	      this->section, this->stage_id );
  
  stg_neighbor_t *nbors;
  size_t ncount = 0;
  assert( stg_get_property( this->stage_client, this->stage_id, 
			    STG_PROP_NEIGHBORS,
			    (void**)&nbors, &ncount ) == 0 );
  
  ncount /= sizeof(stg_neighbor_t);
  
  player_fiducial_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  
  pdata.count = htons((uint16_t)ncount);
  
  for( int i=0; i<(int)ncount; i++ )
    {
      pdata.fiducials[i].id = htons((int16_t)nbors[i].id);
      pdata.fiducials[i].pose[0] = htons((int16_t)(nbors[i].range*1000.0));
      pdata.fiducials[i].pose[1] = htons((int16_t)RTOD(nbors[i].bearing));
      pdata.fiducials[i].pose[2] = htons((int16_t)RTOD(nbors[i].orientation));
      // ignore uncertainty for now
    }
      
  if( nbors )
    free( nbors );

  // publish this data
  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in
  
  //TEST
  /*    const char* str = "hello world";
    stg_los_msg_t msg;
    memset( &msg, 0, sizeof(msg) );
    msg.id = 2; // sonarbot?
    memcpy( &msg.bytes, str, strlen(str));
    msg.len = strlen(str);
    stg_model_send_los_msg( this->stage_client, this->stage_id, &msg );
  */

  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
}

int StgFiducial::PutConfig(player_device_id_t* device, void* client, 
				    void* data, size_t len)
{
  
  // rather than push the request onto the request queue, we'll handle
  // it right away
  
  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
    case PLAYER_FIDUCIAL_GET_GEOM:
      {
	stg_pose_t* org;
	size_t len;
	assert( stg_get_property( this->stage_client, this->stage_id, 
				  STG_PROP_ORIGIN,
				  (void**)&org, &len ) == 0 );
	
	assert( len == sizeof(stg_pose_t) );
	
	stg_size_t* sz;	
	assert( stg_get_property( this->stage_client, this->stage_id, 
				  STG_PROP_SIZE,
				  (void**)&sz, &len ) == 0 );
	
	assert( len == sizeof(stg_size_t) );

	// fill in the geometry data formatted player-like
	player_fiducial_geom_t pgeom;
	pgeom.pose[0] = ntohs((uint16_t)(1000.0 * org->x));
	pgeom.pose[1] = ntohs((uint16_t)(1000.0 * org->y));
	pgeom.pose[2] = ntohs((uint16_t)RTOD(org->a));
	
	pgeom.size[0] = ntohs((uint16_t)(1000.0 * sz->x)); 
	pgeom.size[1] = ntohs((uint16_t)(1000.0 * sz->y)); 
	
	pgeom.fiducial_size[0] = ntohs((uint16_t)100);
	pgeom.fiducial_size[1] = ntohs((uint16_t)100);

	free(org);
	free(sz);

	if( PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		      &pgeom, sizeof(pgeom) )  != 0 )
	  PLAYER_ERROR("PutReply() failed");
      }
      break;
      
    case PLAYER_FIDUCIAL_SEND_MSG:
      {
	assert( len == sizeof( player_fiducial_msg_t) );
	player_fiducial_msg_t* p_msg = (player_fiducial_msg_t*)data;
	
	stg_los_msg_t s_msg;
	memset( &s_msg, 0, sizeof(s_msg) );
	
	s_msg.id = (int32_t)ntohl(p_msg->target_id);
	s_msg.power = (uint16_t)ntohs(p_msg->power);
	s_msg.consume = p_msg->consume;
	
	s_msg.len = (size_t)p_msg->len;

	printf( "sending message len %d\n", s_msg.len );

	if( s_msg.len > STG_LOS_MSG_MAX_LEN ) s_msg.len = STG_LOS_MSG_MAX_LEN;
	memcpy( &s_msg.bytes, p_msg->bytes, s_msg.len );
	
	stg_model_send_los_msg( this->stage_client, this->stage_id, 
				&s_msg );
	
	if( PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, NULL, 0) 
	    != 0 )
	  PLAYER_ERROR("PutReply() failed");;
      }
      break;

    case PLAYER_FIDUCIAL_EXCHANGE_MSG:
      {
	assert( len == sizeof( player_fiducial_msg_t) );
	player_fiducial_msg_t* p_msg = (player_fiducial_msg_t*)data;
	
	stg_los_msg_t s_msg;
	
	s_msg.id = ntohl(p_msg->target_id);
	s_msg.power = ntohs(p_msg->power);
	s_msg.consume = (int)p_msg->consume;
	
	size_t sz = (size_t)p_msg->len;
	if( sz > STG_LOS_MSG_MAX_LEN ) sz =  STG_LOS_MSG_MAX_LEN;
	memcpy( &s_msg.bytes, p_msg->bytes, sz );
	
	stg_model_exchange_los_msg( this->stage_client, this->stage_id, 
					&s_msg );
	
	// re-format the reply into Player-speak

	player_fiducial_msg_t p_reply;
	p_reply.target_id = htonl( s_msg.id );
	p_reply.power = htons( s_msg.power );
	p_reply.consume = (uint8_t)s_msg.consume;
	sz = (size_t)s_msg.len;

	if( sz > PLAYER_FIDUCIAL_MAX_MSG_LEN ) 
	  sz =  PLAYER_FIDUCIAL_MAX_MSG_LEN;
	
	p_reply.len = (uint8_t)s_msg.len;
	memcpy( &p_reply.bytes, &s_msg.bytes, sz );
	
	if (PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		      &p_reply, sizeof(p_reply) ) != 0 )
	  PLAYER_ERROR("PutReply() failed");
      }
    default:
      {
	printf( "Warning: stg_fiducial doesn't support config id %d\n", buf[0] );
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0) 
          PLAYER_ERROR("PutReply() failed");
      }
    }
  
  return(0);
}


