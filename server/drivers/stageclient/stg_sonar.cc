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

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"
#include <stdlib.h>
#include "stageclient.h"

// DRIVER FOR SONAR INTERFACE ///////////////////////////////////////////

class StgSonar:public Stage1p4
{
public:
  StgSonar( ConfigFile* cf, int section );

  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual int PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len);

private:
  // stage has no concept of sonar off/on, so we implement that here
  // to support the sonar power config.
  int power_on;
};

StgSonar::StgSonar( ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_sonar_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgSonar with interface %s", interface );
  
  this->subscribe_list = g_list_append( this->subscribe_list, GINT_TO_POINTER(STG_PROP_RANGERDATA));
    
  power_on = 1; // enabled by default
}

Driver* StgSonar_Init( ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SONAR_STRING))
    {
      PLAYER_ERROR1("driver \"stg_sonar\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((Driver*)(new StgSonar(interface, cf, section)));
}


void StgSonar_Register(DriverTable* table)
{
  table->AddDriver("stg_sonar", PLAYER_ALL_MODE, StgSonar_Init);
}

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgSonar::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{  
  
  stg_property_t* prop = stg_model_get_prop_cached( model, STG_PROP_RANGERDATA);
  
  if( prop )
    {
      stg_ranger_sample_t *rangers = (stg_ranger_sample_t*)prop->data;
      size_t rcount = prop->len / sizeof(stg_ranger_sample_t);
      
      PLAYER_TRACE2( "i see %d bytes of ranger data: %d ranger readings", 
		     (int)prop->len, (int)rcount );
      
      // limit the number of samples to Player's maximum
      if( rcount > PLAYER_SONAR_MAX_SAMPLES )
	rcount = PLAYER_SONAR_MAX_SAMPLES;
      
      player_sonar_data_t sonar;
      memset( &sonar, 0, sizeof(sonar) );
      
      if( power_on ) // set with a sonar config
	{
	  sonar.range_count = htons((uint16_t)rcount);
	  
	  for( int i=0; i<(int)rcount; i++ )
	    sonar.ranges[i] = htons((uint16_t)(1000.0*rangers[i].range));
	}
      
      Driver::PutData( &sonar, sizeof(sonar), 0,0 ); // time gets filled in
    }
  
  // now inherit the standard data-getting behavior 
  return Driver::GetData(client,dest,len,timestamp_sec,timestamp_usec);
}


int StgSonar::PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len)
{
  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
    case PLAYER_SONAR_GET_GEOM_REQ:
      { 
	stg_ranger_config_t* cfgs = NULL;
	size_t len = 0;
	if( stg_model_prop_get_var( this->model, STG_PROP_RANGERCONFIG, 
				    (void**)&cfgs, &len ))
	  {
	    PLAYER_ERROR( "error requesting STG_PROP_RANGERCONFIG" );
	    
	    if(PutReply( device, client, PLAYER_MSGTYPE_RESP_NACK, NULL, 
			 NULL, 0) )
	      PLAYER_ERROR("failed to PutReply for NACK");
	  }
	else
	  PLAYER_TRACE0( "got ranger config OK" );
	
	size_t rcount = len / sizeof(stg_ranger_config_t);
	
	// convert the ranger data into Player-format sonar poses	
	player_sonar_geom_t pgeom;
	memset( &pgeom, 0, sizeof(pgeom) );
	
	pgeom.subtype = PLAYER_SONAR_GET_GEOM_REQ;

	// limit the number of samples to Player's maximum
	if( rcount > PLAYER_SONAR_MAX_SAMPLES )
	  rcount = PLAYER_SONAR_MAX_SAMPLES;

	pgeom.pose_count = htons((uint16_t)rcount);

	for( int i=0; i<(int)rcount; i++ )
	  {
	    // fill in the geometry data formatted player-like
	    pgeom.poses[i][0] = 
	      ntohs((uint16_t)(1000.0 * cfgs[i].pose.x));
	    
	    pgeom.poses[i][1] = 
	      ntohs((uint16_t)(1000.0 * cfgs[i].pose.y));
	    
	    pgeom.poses[i][2] 
	      = ntohs((uint16_t)RTOD( cfgs[i].pose.a));	    
	  }

	  if(PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
	  &pgeom, sizeof(pgeom) ) )
	  PLAYER_ERROR("failed to PutReply");
      }
      break;
      
    case PLAYER_SONAR_POWER_REQ:
      /*
	 * 1 = enable sonars
	 * 0 = disable sonar
	 */
	if( len != sizeof(player_sonar_power_config_t))
	  {
	    PLAYER_WARN2( "stg_sonar: arg to sonar state change "
			  "request wrong size (%d/%d bytes); ignoring",
			  (int)len,(int)sizeof(player_sonar_power_config_t) );
	    
	    if(PutReply( device, client, PLAYER_MSGTYPE_RESP_NACK, 
			NULL, NULL, 0))
	      PLAYER_ERROR("failed to PutReply");
	  }
	
	this->power_on = ((player_sonar_power_config_t*)data)->value;
	
	if(PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		     NULL, 0 ) )
	  PLAYER_ERROR("failed to PutReply");
	
	break;
	
    default:
      {
	printf( "Warning: stg_sonar doesn't support config id %d\n", buf[0] );
	if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
	  PLAYER_ERROR("PutReply() failed");
	break;
      }
      
    }
  
  return(0);
}
