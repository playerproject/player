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

// STAGE-1.4 DRIVER CLASS ///////////////////////////////
//
// creates a single static Stage client. This is subclassed for each
// Player interface. Each instance shares the single Stage connection

#define PLAYER_ENABLE_TRACE 1
#define PLAYER_ENABLE_MSG 1

#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
//#include <sys/soundcard.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

extern int global_argc;
extern char** global_argv;

// took me ages to figure this linkage out. grrr. 
extern "C" {
#include <pam.h> // image-reading library
}

#include "stage1p4.h"
#include "stg_time.h"
extern PlayerTime* GlobalTime;

// init static vars
char* Stage1p4::world_file = DEFAULT_STG_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;
stg_name_id_t* Stage1p4::created_models = NULL;
int Stage1p4::created_models_count = 0;
ConfigFile* Stage1p4::config = NULL;
CWorldFile Stage1p4::wf;
stg_client_t *Stage1p4::subclient = NULL;
stg_id_t Stage1p4::world_id = 0;

// signal cacher - when Player gets a SIG_USR2 we save the worldfile
void catch_sigusr2( int signum )
{
  puts( "PLAYER SAVE" );
  Stage1p4::wf.DownloadAndSave( Stage1p4::stage_client, 
				Stage1p4::created_models,
				Stage1p4::created_models_count );
}

// constructor
//
Stage1p4::Stage1p4(char* interface, ConfigFile* cf, int section, 
		   size_t datasz, size_t cmdsz, int rqlen, int rplen) :
  CDevice(datasz,cmdsz,rqlen,rplen)
{
  PLAYER_TRACE1( "Stage1p4 device created for interface %s\n", interface );
  
  this->config = cf;
  this->section = section; 
  
  // just do this startup stuff once - it is hared by all instances
  // do this in the constructor and not in Setup() so the Stage window
  // gets shown and populated without having to connect a client.
  if( Stage1p4::stage_client == NULL )
    {
      // steal the global clock
      //if( GlobalTime ) delete GlobalTime;
      //assert( (GlobalTime = new StgTime() ));
      
      int stage_port = 
	config->ReadInt(section, "port", STG_DEFAULT_SERVER_PORT);
      
      char* stage_host = 
	(char*)config->ReadString(section, "host", DEFAULT_STG_HOST);
      
      // initialize the bitmap library
      pnm_init( &global_argc, global_argv );

      PLAYER_MSG2( "Creating client to Stage server on %s:%d", 
		   stage_host, stage_port );
      
      Stage1p4::stage_client = 
	stg_client_create( stage_host, stage_port, STG_TOS_REQUESTREPLY );
      assert(Stage1p4::stage_client);
      
      // load a worldfile and create a passel of Stage models
      Stage1p4::world_file = 
	(char*)config->ReadString(section, "worldfile", DEFAULT_STG_WORLDFILE);
      
      PLAYER_MSG1( "Uploading world from \"%s\"", world_file );      
      //CWorldFile wf;
      wf.Load( Stage1p4::world_file );
      // this function from libstagecpp does a lot of work turning the
      // worldfile into a series of model creation and configuration
      // requests. It creates an array of <name,stageid> pairs.
      wf.Upload( Stage1p4::stage_client, 
		 &Stage1p4::created_models, 
		 &Stage1p4::created_models_count,
		 &Stage1p4::world_id );

      // catch USR2 signal. getting this signal makes us SAVE the
      // world state.
      if( signal( SIGUSR2, catch_sigusr2 ) == SIG_ERR )
	PLAYER_ERROR( "stage1p4 failed to install SAVE signal handler." );

      // create another client, this one in subscription mode
      //stg_client_t *subcli = 
      Stage1p4::subclient = 
	stg_client_create( stage_host, stage_port, STG_TOS_SUBSCRIPTION );
      
      assert( Stage1p4::subclient );
      
      stg_subscription_t sub;
      sub.id = Stage1p4::world_id;
      sub.prop = STG_PROP_TIME;
      stg_property_subscribe( Stage1p4::subclient,&sub ); 
      

      this->StartThread();
    } 
}

// destructor
Stage1p4::~Stage1p4()
{
  if( this->stage_client )
    stg_client_free( this->stage_client );
}

int Stage1p4::Setup()
{  
  // Look up my name to get a Stage model id from the array created by
  // the constructor.

  // load my name from the config file
  const char* name = config->ReadString( section, "model", "<no name>" );
  PLAYER_MSG1( "stage1p4 starting device name \"%s\"", name );
  
  // lookup name (todo - speed this up with a hash table)
  this->stage_id = -1; // invalid
  for( int i=0; i < created_models_count; i++ )
    if( strcmp( created_models[i].name, name ) == 0 )
      {
	this->stage_id =  created_models[i].stage_id;
	break;
      }
  
  // handle name match failure
  if( stage_id == -1 )
    {
      PLAYER_ERROR1( "stage1p4: device name \"%s\" doesn't match a Stage model", name );
      return -1; // fail
    }
#ifdef DEBUG
  else
    PLAYER_MSG2( "stage1p4: device name \"%s\" matches stage model %d",
		   name, this->stage_id );    
#endif

  return 0; //ok
}

int Stage1p4::Shutdown()
{
  this->StopThread();

  return 0; //ok
}


void Stage1p4::Main( void )
{
  while(1)
    {
      // test if we are supposed to cancel
      pthread_testcancel();
      //puts( "main" );
      //sleep(2);

      printf( "reading subscribed property on fd %d\n", Stage1p4::subclient->pollfd.fd );
      stg_property_t* prop = stg_property_read( Stage1p4::subclient );

      printf( "received property %d:%d\n",
	      prop->id, prop->property );

      stg_property_free(prop);
    }
}

// END CLASS

