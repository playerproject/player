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

class StgFiducialNeighbors:public Stage1p4
{
public:
  StgFiducialNeighbors(char* interface, ConfigFile* cf, int section );

  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual int PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len);
};

StgFiducialNeighbors::StgFiducialNeighbors(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_fiducial_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgFiducialNeighbors with interface %s", interface );
}

CDevice* StgFiducialNeighbors_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_FIDUCIAL_STRING))
    {
      PLAYER_ERROR1("driver \"stg_fiducial_neighbors\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgFiducialNeighbors(interface, cf, section)));
}


void StgFiducialNeighbors_Register(DriverTable* table)
{
  table->AddDriver("stg_fiducial_neighbors", PLAYER_ALL_MODE, StgFiducialNeighbors_Init);
}

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgFiducialNeighbors::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  PLAYER_MSG2(" STG_FIDUCIAL GETDATA section %d -> model %d",
	      this->section, this->stage_id );
  
  stg_neighbor_t *nbors;
  int count = 0;
  stg_model_get_neighbors( this->stage_client, this->stage_id, &nbors, &count );
  
  player_fiducial_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  
  pdata.count = htons((uint16_t)count);
  
  for( int i=0; i<count; i++ )
    {
      pdata.fiducials[i].id = htons((int16_t)nbors[i].id);
      pdata.fiducials[i].pose[0] = htons((int16_t)(nbors[i].range*1000.0));
      pdata.fiducials[i].pose[1] = htons((int16_t)RTOD(nbors[i].bearing));
      pdata.fiducials[i].pose[2] = htons((int16_t)RTOD(nbors[i].orientation));
      // ignore uncertainty for now
    }
      
  free( nbors );

  // publish this data
  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in
  
  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
}

int StgFiducialNeighbors::PutConfig(player_device_id_t* device, void* client, 
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
	stg_pose_t org;
	stg_model_get_origin( this->stage_client, this->stage_id, &org );
	
	stg_size_t sz;
	stg_model_get_size( this->stage_client, this->stage_id, &sz );
	
	// fill in the geometry data formatted player-like
	player_fiducial_geom_t pgeom;
	pgeom.pose[0] = ntohs((uint16_t)(1000.0 * org.x));
	pgeom.pose[1] = ntohs((uint16_t)(1000.0 * org.y));
	pgeom.pose[2] = ntohs((uint16_t)RTOD(org.a));
	
	pgeom.size[0] = ntohs((uint16_t)(1000.0 * sz.x)); 
	pgeom.size[1] = ntohs((uint16_t)(1000.0 * sz.y)); 
	
	pgeom.fiducial_size[0] = ntohs((uint16_t)100);
	pgeom.fiducial_size[1] = ntohs((uint16_t)100);

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


