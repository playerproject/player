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
  PLAYER_MSG2(" STG_FIDUCIAL GETDATA section %d -> model %d",
	      this->section, this->stage_id );
  
  // request fiducial data from Stage
  
  //stg_transducer_t* trans;
  //int count;
  //stg_model_get_transducers( this->stage_client, this->stage_id, 
  //		     &trans, &count );
  
  stg_neighbor_data_t *nbors;
  int count = 0;
  stg_model_get_neighbors( this->stage_client, this->stage_id, &nbors, &count );
  
  printf( "stage returned %d neighbors", count );
  
  
  player_fiducial_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  

  /*  pdata.min_angle = htons( (int16_t)(RTOD(sdata.angle_min) * 100) );
  pdata.max_angle = htons( (int16_t)(RTOD(sdata.angle_max) * 100) );
  pdata.resolution = htons( (uint16_t)(RTOD(ang_resolution) * 100) );
  pdata.range_count = htons( (uint16_t)sdata.sample_count );
  
  for( int i=0; i< sdata.sample_count; i++ )
    {
      pdata.ranges[i] = htons( (uint16_t)(sdata.samples[i].range * 1000.0 ) );
      pdata.intensity[i] = 0;
    }
  */

  free( nbors );

  // publish this data
  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in
  
  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
  
  //return 0;
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
	// get one fiducial packet, as it contains all the info we need
	stg_fiducial_data_t sdata;
	stg_model_get_fiducial_data( this->stage_client, this->stage_id, &sdata );
		
	// fill in the geometry data formatted player-like
	player_fiducial_geom_t pgeom;
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

