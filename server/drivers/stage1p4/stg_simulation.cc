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
#include "stage1p4.h"

#include "stg_time.h"
extern PlayerTime* GlobalTime;

extern int global_argc;
extern char** global_argv;

// took me ages to figure this linkage out. grrr. 
extern "C" {
#include <pam.h> // image-reading library
}

// DRIVER FOR SIMULATION INTERFACE //////////////////////////////////////////////////////

class StgSimulation:public Stage1p4
{
public:
  StgSimulation(char* interface, ConfigFile* cf, int section);

  //virtual int Setup(){this->StageSubscribe(STG_MOD_SIMULATION); return 0;};
  //virtual int Shutdown(){this->StageUnsubscribe(STG_MOD_SIMULATION); return 0;};
};

StgSimulation::StgSimulation(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, 
	      sizeof(player_simulation_data_t), 
	      sizeof(player_simulation_cmd_t), 1, 1 )
{
  PLAYER_MSG1( "constructing StgSimulation with interface %s", interface );
  
  // just do this startup stuff in the simulation once - it is hared
  // by all instances. do this in the constructor and not in Setup()
  // so the Stage window gets shown and populated without having to
  // connect a client.
  
  if( Stage1p4::stage_client == NULL )
    {
      // steal the global clock - a bit aggressive, but a simple approach
      if( GlobalTime ) delete GlobalTime;
      assert( (GlobalTime = new StgTime(this) ));
      
      Stage1p4::stage_port = 
	config->ReadInt(section, "port", STG_DEFAULT_SERVER_PORT);
      
      Stage1p4::stage_host = 
	(char*)config->ReadString(section, "host", DEFAULT_STG_HOST);
      
      // initialize the bitmap library
      pnm_init( &global_argc, global_argv );

      printf( "Stage1p4: Creating client to Stage server on %s:%d\n", 
	      stage_host, stage_port );
      
      Stage1p4::stage_client = 
	stg_client_create( stage_host, stage_port, STG_TOS_SUBSCRIPTION );
      assert(Stage1p4::stage_client);

      // make another client for configuration requests
      //Stage1p4::stage_config_client = 
      //	stg_client_create( stage_host, stage_port, STG_TOS_SUBSCRIPTION );
      //assert(Stage1p4::stage_config_client);
      
      // load a worldfile
      Stage1p4::world_file = 
	(char*)config->ReadString(section, "worldfile", DEFAULT_STG_WORLDFILE);

      // create a passel of Stage models based on the worldfile
      PLAYER_TRACE1( "Uploading world from \"%s\"", world_file );      
      //CWorldFile wf;
      Stage1p4::wf.Load( Stage1p4::world_file );
      // this function from libstagecpp does a lot of work turning the
      // worldfile into a series of model creation and configuration
      // requests. It creates an array of model records
      Stage1p4::wf.Upload( Stage1p4::stage_client, 
			   &Stage1p4::models, 
			   &Stage1p4::models_count );


      // subscribe to the clock from the world we just created
      stg_property_t* reply = 
	stg_send_property( Stage1p4::stage_client,
			   Stage1p4::models[0].stage_id, // world id
			   STG_WORLD_TIME, 
			   STG_SUBSCRIBE,
			   NULL, 0 );
      
      if( !(reply && (reply->action == STG_SUBSCRIBE) && *reply->data == (char)1 ))
	{
	  PLAYER_ERROR( "stage1p4: time subscription failed" );
	  exit(-1);
	}

      stg_property_free(reply);

      // catch USR2 signal. getting this signal makes us SAVE the
      // world state.
      // TODO - use a network command
      //if( signal( SIGUSR2, catch_sigusr2 ) == SIG_ERR )
      //PLAYER_ERROR( "stage1p4 failed to install SAVE signal handler." );
    } 
  
}

CDevice* StgSimulation_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SIMULATION_STRING))
    {
      PLAYER_ERROR1("driver \"stg_simulation\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgSimulation(interface, cf, section)));
}


void StgSimulation_Register(DriverTable* table)
{
  table->AddDriver("stg_simulation", PLAYER_ALL_MODE, StgSimulation_Init);
}

//////////////////////////////////////////////////////////////////////////////


