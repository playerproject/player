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

// DRIVER FOR LASER INTERFACE ////////////////////////////////////////////////////////

class StgLaser:public Stage1p4
{
public:
  StgLaser(char* interface, ConfigFile* cf, int section );

  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual int PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len);
};

StgLaser::StgLaser(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_laser_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgLaser with interface %s", interface );
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

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgLaser::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  PLAYER_MSG2(" STG_LASER GETDATA section %d -> model %d",
	      this->section, this->stage_id );
  
  // request laser data from Stage
  
  //stg_transducer_t* trans;
  //int count;
  //stg_model_get_transducers( this->stage_client, this->stage_id, 
  //		     &trans, &count );
  
  stg_laser_data_t sdata;
  stg_model_get_laser_data( this->stage_client, this->stage_id, &sdata );
  
  player_laser_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  
  double ang_resolution = (sdata.angle_max - sdata.angle_min)/sdata.sample_count;
  pdata.min_angle = htons( (int16_t)(RTOD(sdata.angle_min) * 100) );
  pdata.max_angle = htons( (int16_t)(RTOD(sdata.angle_max) * 100) );
  pdata.resolution = htons( (uint16_t)(RTOD(ang_resolution) * 100) );
  pdata.range_count = htons( (uint16_t)sdata.sample_count );
  
  for( int i=0; i< sdata.sample_count; i++ )
    {
      pdata.ranges[i] = htons( (uint16_t)(sdata.samples[i].range * 1000.0 ) );
      pdata.intensity[i] = 0;
    }

  // publish this data
  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in
  
  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
  
  //return 0;
}

int StgLaser::PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len)
{
  
  // rather than push the request onto the request queue, we'll handle
  // it right away
  
  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
    case PLAYER_LASER_GET_GEOM:
      {
	// get one laser packet, as it contains all the info we need
	stg_laser_data_t sdata;
	stg_model_get_laser_data( this->stage_client, this->stage_id, &sdata );
		
	// fill in the geometry data formatted player-like
	player_laser_geom_t pgeom;
	pgeom.pose[0] = ntohs((uint16_t)(1000.0 * sdata.pose.x));
	pgeom.pose[1] = ntohs((uint16_t)(1000.0 * sdata.pose.y));
	pgeom.pose[2] = ntohs((uint16_t)RTOD( sdata.pose.a));
	
	pgeom.size[0] = ntohs((uint16_t)(1000.0 * sdata.size.x)); 
	pgeom.size[1] = ntohs((uint16_t)(1000.0 * sdata.size.y)); 
	
	PutReply( device, client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
		  &pgeom, sizeof(pgeom) );
      }
      break;
      
    default:
      PLAYER_WARN1( "stage1p4 doesn't support config id %d", buf[0] );
      break;
    }

  return(0);
}

