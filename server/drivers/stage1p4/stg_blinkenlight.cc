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
#include "stage1p4.h"

// CLASS FOR BLINKENLIGHT INTERFACE //////////////////////////////////////////
class StgBlinkenlight:public Stage1p4
{
public:
  StgBlinkenlight(char* interface, ConfigFile* cf, int section );
  
  // override GetData
  virtual size_t GetData(void* client, unsigned char* dest, size_t len,
			 uint32_t* timestamp_sec, uint32_t* timestamp_usec);

  // overrride PutCommand
  virtual void PutCommand(void* client, unsigned char* src, size_t len);

  virtual int Setup(){this->StageSubscribe(STG_MOD_BLINKENLIGHT); return 0;};
  virtual int Shutdown(){this->StageUnsubscribe(STG_MOD_BLINKENLIGHT); return 0;};
  
  protected:
};


// METHODS ///////////////////////////////////////////////////////////////

StgBlinkenlight::StgBlinkenlight(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, 
	      sizeof(player_blinkenlight_data_t), sizeof(player_blinkenlight_cmd_t), 0, 0 )
{
  PLAYER_MSG0( "STG_BLINKENLIGHT CONSTRUCTOR" );
  
  PLAYER_TRACE1( "constructing StgBlinkenlight with interface %s", interface );
  
}

CDevice* StgBlinkenlight_Init(char* interface, ConfigFile* cf, int section)
{
  PLAYER_MSG0( "STG_BLINKENLIGHT INIT" );
    
  if(strcmp(interface, PLAYER_BLINKENLIGHT_STRING))
    {
      PLAYER_ERROR1("driver \"stg_blinkenlight\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgBlinkenlight(interface, cf, section)));
}
	      
void StgBlinkenlight_Register(DriverTable* table)
{
  table->AddDriver("stg_blinkenlight", PLAYER_ALL_MODE, StgBlinkenlight_Init);
}

// override GetData to get data from Stage on demand, rather than the
// standard model of the source filling a buffer periodically
size_t StgBlinkenlight::GetData(void* client, unsigned char* dest, size_t len,
			    uint32_t* timestamp_sec, uint32_t* timestamp_usec)
{
  stg_model_t* model = &Stage1p4::models[this->section];

  PLAYER_MSG2(" STG_BLINKENLIGHT GETDATA section %d -> model %d",
	      this->section, model->stage_id );
    
  this->WaitForData( model->stage_id, STG_MOD_RANGERS );
  
  stg_blinkenlight_t* bl = (stg_blinkenlight_t*)
    model->props[STG_MOD_BLINKENLIGHT]->data;
  size_t blen = model->props[STG_MOD_BLINKENLIGHT]->len;
  
  assert(bl);
  assert( blen == sizeof(stg_blinkenlight_t) );
  
  // pack the data into player format
  player_blinkenlight_data_t pdata;
  memset( &pdata, 0, sizeof(pdata) );
  pdata.enable = bl->enable;
  pdata.period_ms = htons((uint16_t)bl->period_ms);
  
  // publish this data
  CDevice::PutData( &pdata, sizeof(pdata), 0,0 ); // time gets filled in

  // now inherit the standard data-getting behavior 
  return CDevice::GetData(client,dest,len,timestamp_sec,timestamp_usec);
  
  //return 0;
}

void  StgBlinkenlight::PutCommand(void* client, unsigned char* src, size_t len)
{
  assert( len == sizeof(player_blinkenlight_cmd_t) );
  
  // convert from Player to Stage format
  player_blinkenlight_cmd_t* pcmd = (player_blinkenlight_cmd_t*)src;
  
  stg_blinkenlight_t sb;
  sb.enable = (int)pcmd->enable;
  sb.period_ms = (uint16_t)ntohs(pcmd->period_ms);
  
  assert( stg_set_property( this->stage_client, 
			    Stage1p4::models[this->section].stage_id,
			    STG_MOD_BLINKENLIGHT, 
			    (void*)&sb, sizeof(sb) ) 
	  == 0 );
}

