/*
 *  Stage-1.4 driver for Player
 *  Copyright (C) 2003  Richard Vaughan (rtv) vaughan@hrl.com 
 *  for the Player/Stage Project http://playerstage.sourceforge.net
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

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 1

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"
#include "stageclient.h"

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
      PLAYER_ERROR1("driver \"stg_fiducial\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgFiducial(interface, cf, section)));
}


void StgFiducial_Register(DriverTable* table)
{
  table->AddDriver("stg_fiducial", PLAYER_ALL_MODE, StgFiducial_Init);
}

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgFiducial::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  PLAYER_TRACE2(" STG_FIDUCIAL GETDATA section %d -> model %d",
		model->section, model->id_client );
  
  stg_property_t* prop = stg_model_get_prop_cached( model, STG_PROP_DATA);

  player_fiducial_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  
  if( prop && prop->len > 0 )
    {
      stg_fiducial_t *fids = (stg_fiducial_t*)prop->data;    
      size_t fcount = prop->len / sizeof(stg_fiducial_t);      
      assert( fcount > 0 );
      
      pdata.count = htons((uint16_t)fcount);
      
      for( int i=0; i<(int)fcount; i++ )
	{
	  pdata.fiducials[i].id = htons((int16_t)fids[i].id);
	  pdata.fiducials[i].pose[0] = htons((int16_t)(fids[i].range*1000.0));
	  pdata.fiducials[i].pose[1] = htons((int16_t)RTOD(fids[i].bearing));
	  pdata.fiducials[i].pose[2] = htons((int16_t)RTOD(fids[i].geom.a));	      
	  // player can't handle per-fiducial size.
	  // we leave uncertainty (upose) at zero
	}
    }
  
  // publish this data
  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in
  
  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
}

int StgFiducial::PutConfig(player_device_id_t* device, void* client, 
			   void* data, size_t len)
{
  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
    case PLAYER_FIDUCIAL_GET_GEOM:
      {	
	PLAYER_TRACE0( "requesting fiducial geom" );

	// just get the model's geom - Stage doesn't have separate
	// fiducial geom (yet)
	stg_geom_t geom;
	if( stg_model_prop_get( this->model, STG_PROP_GEOM, &geom,sizeof(geom)))
	  PLAYER_ERROR( "error requesting STG_PROP_GEOM" );
	else
	  PLAYER_TRACE0( "got fiducial geom OK" );
      
	// fill in the geometry data formatted player-like
	player_fiducial_geom_t pgeom;
	pgeom.pose[0] = htons((uint16_t)(1000.0 * geom.pose.x));
	pgeom.pose[1] = htons((uint16_t)(1000.0 * geom.pose.y));
	pgeom.pose[2] = htons((uint16_t)RTOD( geom.pose.a));
	
	pgeom.size[0] = htons((uint16_t)(1000.0 * geom.size.x)); 
	pgeom.size[1] = htons((uint16_t)(1000.0 * geom.size.y)); 
	
	pgeom.fiducial_size[0] = ntohs((uint16_t)100); // TODO - get this info
	pgeom.fiducial_size[1] = ntohs((uint16_t)100);
	
	if( PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		      &pgeom, sizeof(pgeom) ) != 0 )
	  PLAYER_ERROR("PutReply() failed for PLAYER_LASER_GET_GEOM");      
      }
      break;
      
    case PLAYER_FIDUCIAL_SET_FOV:
      
      if( len == sizeof(player_fiducial_fov_t) )
	{
	  player_fiducial_fov_t* pfov = (player_fiducial_fov_t*)data;
	  
	  // convert from player to stage FOV packets
	  stg_fiducial_config_t setcfg;
	  memset( &setcfg, 0, sizeof(setcfg) );
	  setcfg.min_range = (uint16_t)ntohs(pfov->min_range) / 1000.0;
	  setcfg.max_range_id = (uint16_t)ntohs(pfov->max_range) / 1000.0;
	  setcfg.max_range_anon = setcfg.max_range_id;
	  setcfg.fov = DTOR((uint16_t)ntohs(pfov->view_angle));
	  
	  //printf( "setting fiducial FOV to min %f max %f fov %f\n",
	  //  setcfg.min_range, setcfg.max_range_anon, setcfg.fov );
	  
	  if( stg_model_prop_set( this->model, STG_PROP_CONFIG, 
				  &setcfg,sizeof(setcfg)))
	    PLAYER_ERROR( "error setting fiducial STG_PROP_CONFIG" );
	  else
	    PLAYER_TRACE0( "set fiducial config OK" );
	}    
      else
	PLAYER_ERROR2("Incorrect packet size setting fiducial FOV (%d/%d)",
		      (int)len, (int)sizeof(player_fiducial_fov_t) );      
      
      // deliberate no-break - SET_FOV needs the current FOV as a reply
      
    case PLAYER_FIDUCIAL_GET_FOV:
      {
	PLAYER_TRACE0( "requesting fiducial FOV" );
	
	stg_fiducial_config_t cfg;
	if( stg_model_prop_get( this->model, STG_PROP_CONFIG, 
				&cfg,sizeof(cfg))
	    != 0 )
	  PLAYER_TRACE0( "error requesting STG_PROP_CONFIG" );
	
	
	// fill in the geometry data formatted player-like
	player_fiducial_fov_t pfov;
	pfov.min_range = htons((uint16_t)(1000.0 * cfg.min_range));
	pfov.max_range = htons((uint16_t)(1000.0 * cfg.max_range_anon));
	pfov.view_angle = htons((uint16_t)RTOD(cfg.fov));
	
	if( PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		      &pfov, sizeof(pfov) ) != 0 )
	  PLAYER_ERROR("PutReply() failed for "
		       "PLAYER_FIDUCIAL_GET_FOV or PLAYER_FIDUCIAL_SET_FOV");      
      }
      break;
      
    case PLAYER_FIDUCIAL_SET_ID:
      
      if( len == sizeof(player_fiducial_id_t) )
	{
	  PLAYER_TRACE0( "setting fiducial id" );
	  
	  int id = ntohl(((player_fiducial_id_t*)data)->id);
	  
	  if( stg_model_prop_set( this->model, STG_PROP_FIDUCIALRETURN, 
				  &id,sizeof(id)))
	    PLAYER_ERROR( "error setting STG_PROP_FIDUCIALRETURN" );
	  else
	    PLAYER_TRACE0( "set fiducial id OK" );
	}
      else
	PLAYER_ERROR2("Incorrect packet size setting fiducial ID (%d/%d)",
		      (int)len, (int)sizeof(player_fiducial_id_t) );      
      
      // deliberate no-break - SET_ID needs the current ID as a reply

  case PLAYER_FIDUCIAL_GET_ID:
      {
	PLAYER_TRACE0( "requesting fiducial ID" );
	puts( "requesting fiducial ID" );
	
	int id = 0;
	if( stg_model_prop_get( this->model, STG_PROP_FIDUCIALRETURN, 
				&id,sizeof(id))
	    != 0 )
	  PLAYER_TRACE0( "error requesting STG_PROP_FIDUCIALRETURN" );
		
	// fill in the data formatted player-like
	player_fiducial_id_t pid;
	pid.id = htonl(id);
	
	if( PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		      &pid, sizeof(pid) ) != 0 )
	  PLAYER_ERROR("PutReply() failed for "
		       "PLAYER_FIDUCIAL_GET_ID or PLAYER_FIDUCIAL_SET_ID");      
      }
      break;


    default:
      {
	printf( "Warning: stg_fiducial doesn't support config id %d\n", buf[0] );
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0) 
          PLAYER_ERROR("PutReply() failed");
      }
    }
  
  return(0);
}


