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
#include "stage1p4.h"

// DRIVER FOR SONAR INTERFACE ///////////////////////////////////////////

class StgBlobfinder:public Stage1p4
{
public:
  StgBlobfinder(char* interface, ConfigFile* cf, int section );

  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  virtual int PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len);

private:


};

StgBlobfinder::StgBlobfinder(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, sizeof(player_blobfinder_data_t), 0, 1, 1 )
{
  PLAYER_TRACE1( "constructing StgBlobfinder with interface %s", interface );

  this->subscribe_prop = STG_PROP_BLOBS;
   
}

CDevice* StgBlobfinder_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_BLOBFINDER_STRING))
    {
      PLAYER_ERROR1("driver \"stg_blobfinder\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgBlobfinder(interface, cf, section)));
}


void StgBlobfinder_Register(DriverTable* table)
{
  table->AddDriver("stg_blobfinder", PLAYER_ALL_MODE, StgBlobfinder_Init);
}

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgBlobfinder::GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{  
  
  stg_property_t* prop = stg_model_get_prop_cached( model, this->subscribe_prop);
  
  if( prop )
    {
      stg_blobfinder_blob_t *blobs = (stg_blobfinder_blob_t*)prop->data;
      size_t bcount = prop->len / sizeof(stg_blobfinder_blob_t);
      
      PLAYER_TRACE2( "i see %d bytes of blob data: %d blobs", 
		     (int)prop->len, (int)bcount );
      
      // limit the number of samples to Player's maximum
      //if( rcount > PLAYER_BLOBFINDER_MAX_SAMPLES )
      //rcount = PLAYER_BLOBFINDER_MAX_SAMPLES;
      
      player_blobfinder_data_t bfd;
      memset( &bfd, 0, sizeof(bfd) );
      
      bfd.width = 180;
      bfd.height = 120;
      bfd.header[0].index = 0;
      bfd.header[0].num = bcount;
      
      for( int b=0; b<bcount; b++ )
	{
	  bfd.blobs[b].x      = htons((uint16_t)blobs[b].xpos);
	  bfd.blobs[b].y      = htons((uint16_t)blobs[b].ypos);
	  bfd.blobs[b].left   = htons((uint16_t)blobs[b].left);
	  bfd.blobs[b].right  = htons((uint16_t)blobs[b].right);
	  bfd.blobs[b].top    = htons((uint16_t)blobs[b].top);
	  bfd.blobs[b].bottom = htons((uint16_t)blobs[b].bottom);
	  
	  bfd.blobs[b].color = htonl(blobs[b].color);
	  bfd.blobs[b].area  = htonl(blobs[b].area);
	}

      CDevice::PutData( &bfd, sizeof(bfd), 0,0 ); // time gets filled in
    }
  
  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
}


int StgBlobfinder::PutConfig(player_device_id_t* device, void* client, 
			void* data, size_t len)
{
  // switch on the config type (first byte)
  uint8_t* buf = (uint8_t*)data;
  switch( buf[0] )
    {  
    default:
      {
	printf( "Warning: stg_blobfinder doesn't support config id %d\n", buf[0] );
	if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
	  PLAYER_ERROR("PutReply() failed");
	break;
      }
      
    }
  
  return(0);
}
