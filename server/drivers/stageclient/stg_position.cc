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

#include <stdlib.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"
#include "stageclient.h"

// CLASS FOR POSITION INTERFACE //////////////////////////////////////////
class StgPosition:public Stage1p4
{
public:
  StgPosition(char* interface, ConfigFile* cf, int section );
  
  // override GetData
  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // overrride PutCommand
  virtual void PutCommand(void* client, unsigned char* src, size_t len);
    
  // override PutConfig
    virtual int PutConfig(player_device_id_t* device, void* client, 
			  void* data, size_t len);
protected:
  
  //player_position_data_t position_data;
  //stg_pose_t geom_pose;
  //stg_size_t geom_size;
};


// METHODS ///////////////////////////////////////////////////////////////

StgPosition::StgPosition(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, 
	      sizeof(player_position_data_t), sizeof(player_position_cmd_t), 1, 1 )
{
  PLAYER_MSG0( "STG_POSITION CONSTRUCTOR" );
  
  PLAYER_TRACE1( "constructing StgPosition with interface %s", interface );
}

CDevice* StgPosition_Init(char* interface, ConfigFile* cf, int section)
{
  PLAYER_MSG0( "STG_POSITION INIT" );
    
  if(strcmp(interface, PLAYER_POSITION_STRING))
    {
      PLAYER_ERROR1("driver \"stg_position\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgPosition(interface, cf, section)));
}
	      
void StgPosition_Register(DriverTable* table)
{
  table->AddDriver("stg_position", PLAYER_ALL_MODE, StgPosition_Init);
}

size_t StgPosition::GetData(void* client, unsigned char* dest, size_t len,
			    uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  stg_property_t* prop = stg_model_get_prop_cached( model, STG_PROP_DATA );
  
  if( prop && prop->len == sizeof(stg_position_data_t) )      
    {      
      stg_position_data_t* posdat = (stg_position_data_t*)prop->data;
      
      //PLAYER_MSG3( "get data posdat %.2f %.2f %.2f", posdat->x, posdat->y, posdat->a );
      player_position_data_t position_data;
      memset( &position_data, 0, sizeof(position_data) );
      // pack the data into player format
      position_data.xpos = ntohl((int32_t)(1000.0 * posdat->pose.x));
      position_data.ypos = ntohl((int32_t)(1000.0 * posdat->pose.y));
      position_data.yaw = ntohl((int32_t)(RTOD(posdat->pose.a)));
      
      // speeds
      position_data.xspeed= ntohl((int32_t)(1000.0 * posdat->velocity.x)); // mm/sec
      position_data.yspeed = ntohl((int32_t)(1000.0 * posdat->velocity.y));// mm/sec
      position_data.yawspeed = ntohl((int32_t)(RTOD(posdat->velocity.a))); // deg/sec
      
      position_data.stall = posdat->stall;// ? 1 : 0;
      
      //printf( "getdata called at %lu ms\n", stage_client->stagetime );
      
      // publish this data
      CDevice::PutData( &position_data, sizeof(position_data), 0,0 ); 
    }
  
      // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
}
  
void  StgPosition::PutCommand(void* client, unsigned char* src, size_t len)
{
  if( len == sizeof(player_position_cmd_t) )
    {
      // convert from Player to Stage format
      player_position_cmd_t* pcmd = (player_position_cmd_t*)src;

      // only velocity control mode works yet
      stg_position_cmd_t cmd; 
      cmd.x = ((double)((int32_t)ntohl(pcmd->xspeed))) / 1000.0;
      cmd.y = ((double)((int32_t)ntohl(pcmd->yspeed))) / 1000.0;
      cmd.a = DTOR((double)((int32_t)ntohl(pcmd->yawspeed)));
      cmd.mode = STG_POSITION_CONTROL_VELOCITY;

      stg_model_prop_delta( this->model, STG_PROP_COMMAND, 
			    &cmd, sizeof(cmd) ) ;
    }
  else
    PLAYER_ERROR2( "wrong size position command packet (%d/%d bytes)",
		   (int)len, (int)sizeof(player_position_cmd_t) );
}


int StgPosition::PutConfig(player_device_id_t* device, void* client, 
			   void* data, size_t len)
{
  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
    case PLAYER_POSITION_GET_GEOM_REQ:
      {
	stg_geom_t geom;
	if( stg_model_prop_get( this->model, STG_PROP_GEOM, &geom,sizeof(geom)))
	  PLAYER_ERROR( "error requesting STG_PROP_GEOM" );
	
	// fill in the geometry data formatted player-like
	player_position_geom_t pgeom;
	pgeom.pose[0] = ntohs((uint16_t)(1000.0 * geom.pose.x));
	pgeom.pose[1] = ntohs((uint16_t)(1000.0 * geom.pose.y));
	pgeom.pose[2] = ntohs((uint16_t)RTOD(geom.pose.a));
	
	pgeom.size[0] = ntohs((uint16_t)(1000.0 * geom.size.x)); 
	pgeom.size[1] = ntohs((uint16_t)(1000.0 * geom.size.y)); 
	
	PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		  &pgeom, sizeof(pgeom) );
      }
      break;

    default:
      {
	PLAYER_WARN1( "stage1p4 doesn't support config id %d", buf[0] );
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
      }
    }
  
  return(0);
}

