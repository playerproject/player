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

#define PLAYER_ENABLE_TRACE 0
#define PLAYER_ENABLE_MSG 1

#include <stdlib.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"
#include "stageclient.h"
#include "stage.h"

// DRIVER FOR LASER INTERFACE ////////////////////////////////////////////////////////

class StgLaser:public Stage1p4
{
public:
  StgLaser(char* interface, ConfigFile* cf, int section );

  virtual int Setup();
  virtual int Shutdown(); 

  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual int PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len);
};


StgLaser::StgLaser(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_laser_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgLaser with interface %s", interface );
  
  //this->subscribe_prop = STG_PROP_LASERDATA;
  this->subscribe_list = g_list_append( this->subscribe_list, GINT_TO_POINTER(STG_PROP_LASERDATA));

}

CDevice* StgLaser_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_LASER_STRING))
    {
      PLAYER_ERROR1("driver \"stg_laser\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgLaser(interface, cf, section)));
}


void StgLaser_Register(DriverTable* table)
{
  table->AddDriver("stg_laser", PLAYER_ALL_MODE, StgLaser_Init);
}


int StgLaser::Setup()
{
  // subscribe to laser config as well as our regular data
  stg_model_subscribe( this->model, STG_PROP_LASERCONFIG, 100 ); // 100ms    
  
  return Stage1p4::Setup();
}

int StgLaser::Shutdown()
{
  // unsubscribe to laser config
  stg_model_unsubscribe( this->model, STG_PROP_LASERCONFIG );  
  
  return Stage1p4::Shutdown();
}


size_t StgLaser::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  stg_property_t* prop = stg_model_get_prop_cached( model, STG_PROP_LASERDATA);
  
  player_laser_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  
  if( prop ) 
    {      
      //stg_laser_config_t* cfg = (stg_laser_config_t*)prop->data;
      stg_laser_sample_t* samples = (stg_laser_sample_t*)prop->data;
      //	(((char*)prop->data) + sizeof(stg_laser_config_t));
      
      int sample_count = prop->len / sizeof( stg_laser_sample_t );

      // we get the config to stick into Player's packet
      stg_property_t* prop = 
	stg_model_get_prop_cached( model, STG_PROP_LASERCONFIG );
      assert( prop );
      
      stg_laser_config_t* cfg = (stg_laser_config_t*)prop->data;

      if( sample_count != cfg->samples )
	{
	  PRINT_ERR2( "bad laser data: got %d/%d samples",
		      sample_count, cfg->samples );
	}
      else
	{      
	  double min_angle = -RTOD(cfg->fov/2.0);
	  double max_angle = +RTOD(cfg->fov/2.0);
	  double angular_resolution = RTOD(cfg->fov / cfg->samples);
	  double range_multiplier = 1; // TODO - support multipliers
	  
	  pdata.min_angle = htons( (int16_t)(min_angle * 100 )); // degrees / 100
	  pdata.max_angle = htons( (int16_t)(max_angle * 100 )); // degrees / 100
	  pdata.resolution = htons( (uint16_t)(angular_resolution * 100)); // degrees / 100
	  pdata.range_res = htons( (uint16_t)range_multiplier);
	  pdata.range_count = htons( (uint16_t)cfg->samples );
	  
	  for( int i=0; i<cfg->samples; i++ )
	    {
	      //printf( "range %d %d\n", i, samples[i].range);
	      
	      pdata.ranges[i] = htons( (uint16_t)(samples[i].range) );
	      pdata.intensity[i] = 0;
	    }
	  
	  // publish this data
	  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in
	}
    }
 
  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
  
  //return 0;
}


int StgLaser::PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len)
{
  assert( data );

  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
     case PLAYER_LASER_SET_CONFIG:
      {
	player_laser_config_t* plc = (player_laser_config_t*)data;
	
        if( len == sizeof(player_laser_config_t) )
	  {
	    stg_laser_config_t slc_request;
	    memset( &slc_request, 0, sizeof(slc_request) );
	    int min_a = (int16_t)ntohs(plc->min_angle);
	    int max_a = (int16_t)ntohs(plc->max_angle);
	    int ang_res = (int16_t)ntohs(plc->resolution);
	    // TODO
	    // int intensity = plc->intensity;
	    
	    PRINT_DEBUG4( "requested laser config:\n %d %d %d %d",
			  min_a, max_a, ang_res, intensity );
	    
	    min_a /= 100;
	    max_a /= 100;
	    ang_res /= 100;

	    slc_request.fov = DTOR(max_a - min_a);
	    // todo - slc_request.intensity = plc->intensity;

	    slc_request.samples = (int)(slc_request.fov / DTOR(ang_res));
	    	    
	    int err;
	    if( (err = stg_model_prop_set( this->model, STG_PROP_LASERCONFIG,
					   &slc_request, sizeof(slc_request)) ) )
	      PLAYER_ERROR1( "error %d setting laser config", err );
	    else
	      PLAYER_TRACE0( "set laser config OK" );

	    if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, plc, len) != 0)
	      PLAYER_ERROR("PutReply() failed for PLAYER_LASER_SET_CONFIG");
	  }
	else
	  {
	    PLAYER_ERROR2("config request len is invalid (%d != %d)", 
			  (int)len, (int)sizeof(player_laser_config_t));
	    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
	      PLAYER_ERROR("PutReply() failed for PLAYER_LASER_SET_CONFIG");
	  }
      }
      break;
      
    case PLAYER_LASER_GET_CONFIG:
      {   
	if( len == 1 )
	  {
	    stg_laser_config_t slc;
	    if( stg_model_prop_get( this->model, STG_PROP_LASERCONFIG, &slc,sizeof(slc))
		!= 0 )
	      PLAYER_TRACE0( "error requesting STG_PROP_LASERCONFIG" );
	    
	    //stg_print_laser_config( &slc );
	    
	    // integer angles in degrees/100
	    int16_t  min_angle = (int16_t)(-RTOD(slc.fov/2.0) * 100);
	    int16_t  max_angle = (int16_t)(+RTOD(slc.fov/2.0) * 100);
	    //uint16_t angular_resolution = RTOD(slc.fov / slc.samples) * 100;
	    uint16_t angular_resolution =(uint16_t)(RTOD(slc.fov / (slc.samples-1)) * 100);
	    uint16_t range_multiplier = 1; // TODO - support multipliers

	    int intensity = 1; // todo

	    //printf( "laser config:\n %d %d %d %d\n",
	    //	    min_angle, max_angle, angular_resolution, intensity );
	    
	    player_laser_config_t plc;
	    memset(&plc,0,sizeof(plc));
	    plc.min_angle = htons(min_angle); 
	    plc.max_angle = htons(max_angle); 
	    plc.resolution = htons(angular_resolution);
	    plc.range_res = htons(range_multiplier);
	    plc.intensity = intensity;

	    if(PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &plc, 
			sizeof(plc)) != 0)
	      PLAYER_ERROR("PutReply() failed for PLAYER_LASER_GET_CONFIG");      
	  }
	else
	  {
	    PLAYER_ERROR2("config request len is invalid (%d != %d)", (int)len, 1);
	    if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
	      PLAYER_ERROR("PutReply() failed for PLAYER_LASER_GET_CONFIG");
	  }
      }
      break;

    case PLAYER_LASER_GET_GEOM:
      {	
	PLAYER_TRACE0( "requesting laser geom" );

	stg_laser_config_t slc;
	if( stg_model_prop_get( this->model, STG_PROP_LASERCONFIG, &slc,sizeof(slc))
	    != 0 )
	  PLAYER_TRACE0( "error requesting STG_PROP_LASERCONFIG" );
	
      
	// fill in the geometry data formatted player-like
	player_laser_geom_t pgeom;
	pgeom.pose[0] = htons((uint16_t)(1000.0 * slc.geom.pose.x));
	pgeom.pose[1] = htons((uint16_t)(1000.0 * slc.geom.pose.y));
	pgeom.pose[2] = htons((uint16_t)RTOD( slc.geom.pose.a));
	
	pgeom.size[0] = htons((uint16_t)(1000.0 * slc.geom.size.x)); 
	pgeom.size[1] = htons((uint16_t)(1000.0 * slc.geom.size.y)); 
	
	if( PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		      &pgeom, sizeof(pgeom) ) != 0 )
	  PLAYER_ERROR("PutReply() failed for PLAYER_LASER_GET_GEOM");      
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

