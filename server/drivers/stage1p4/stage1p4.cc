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

// STAGE-1.4 DRIVER BASE CLASS  /////////////////////////////////
// creates a single static Stage client

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
#include <sys/soundcard.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>

#include <pthread.h>

extern int global_argc;
extern char** global_argv;

// took me ages to figure this linkage out. grrr. 
extern "C" {
#include <pam.h> // image-reading library
}

#include "stage1p4.h"

// init static vars
char* Stage1p4::world_file = DEFAULT_STG_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;
stg_name_id_t* Stage1p4::created_models = NULL;
int Stage1p4::created_models_count = 0;


// constructor
//
Stage1p4::Stage1p4(char* interface, ConfigFile* cf, int section, 
		   size_t datasz, size_t cmdsz, int rqlen, int rplen) :
  CDevice(datasz,cmdsz,rqlen,rplen)
{
  PLAYER_TRACE1( "Stage1p4 device created for interface %s\n", interface );
  
  this->section = section;
  
  // load my name from the config file
  const char* name = cf->ReadString( section, "name", "<no name>" );
  
  PLAYER_MSG1( "stage1p4 creating device name \"%s\"",
	       name );
  
  if(  Stage1p4::stage_client == NULL )
    {
      Stage1p4::world_file = 
	(char*)cf->ReadString(section, "worldfile", DEFAULT_STG_WORLDFILE);
      
      int stage_port = 
	cf->ReadInt(section, "port", STG_DEFAULT_SERVER_PORT);
      
      char* stage_host = 
	(char*)cf->ReadString(section, "host", DEFAULT_STG_HOST);
      
      //Stage1p4::name_id_table = 
      //g_hash_table_new( g_str_hash, g_str_equal );
      
      Stage1p4::stage_client = 
	this->CreateStageClient(stage_host, stage_port, world_file);	
    }
  
  // now the Stage worldfile has been read and all the devices created.
  // I look up my name to get a Stage model id.

  this->stage_id = -1; // invalid
  for( int i=0; i < created_models_count; i++ )
    if( strcmp( created_models[i].name, name ) == 0 )
      {
	this->stage_id =  created_models[i].stage_id;
	break;
      }
  
    if( stage_id == -1 )
      PLAYER_ERROR1( "stage1p4: device name \"%s\" doesn't match a Stage model", name );
    else
      PLAYER_MSG2( "stage1p4: device name \"%s\" matches stage model %d",
		   name, this->stage_id );    
}

// destructor
Stage1p4::~Stage1p4()
{
  if( this->stage_client )
    this->DestroyStageClient( this->stage_client );
}

stg_client_t* Stage1p4::CreateStageClient( char* host, int port, char* world )
{
  // initialize the bitmap library
  pnm_init( &global_argc, global_argv );

  PLAYER_MSG2( "Creating client to Stage server on %s:%d", host, port );

  stg_client_t* cli = stg_client_create( host, port );
  assert( cli );

  PLAYER_MSG1( "Uploading world from \"%s\"", world );

  // TODO - move all this into the worldfile library
  // load a worldfile
  CWorldFile wf;
  wf.Load( world );
  
  stg_world_create_t world_cfg;
  strncpy(world_cfg.name, wf.ReadString( 0, "name", world ), STG_TOKEN_MAX );
  strncpy(world_cfg.token, this->world_file , STG_TOKEN_MAX );
  world_cfg.width =  wf.ReadTupleFloat( 0, "size", 0, 10.0 );
  world_cfg.height =  wf.ReadTupleFloat( 0, "size", 1, 10.0 );
  world_cfg.resolution = wf.ReadFloat( 0, "resolution", 0.1 );
  stg_id_t root = stg_world_create( cli, &world_cfg );
  
  // for every worldfile section, we may need to store a model ID in
  // order to resolve parents
  created_models_count = wf.GetEntityCount();
  created_models = new stg_name_id_t[ created_models_count ];
  
  // the default parent of every model is root
  for( int m=0; m<created_models_count; m++ )
    {
      created_models[m].stage_id = root;
      strncpy( created_models[m].name, "root", STG_TOKEN_MAX );
    }

  // Iterate through sections and create entities as required
  for (int section = 1; section < wf.GetEntityCount(); section++)
  {
    if( strcmp( "gui", wf.GetEntityType(section) ) == 0 )
      {
	PLAYER_WARN( "gui section not implemented" );
      }
    else
      {
	// TODO - handle the line numbers
	const int line = wf.ReadInt(section, "line", -1);

	stg_id_t parent = created_models[wf.GetEntityParent(section)].stage_id;

	PRINT_MSG1( "creating child of parent %d", parent );
	
	stg_entity_create_t child;
	strncpy(child.name, wf.ReadString(section,"name", "" ), 
		STG_TOKEN_MAX);
	strncpy(child.token, wf.GetEntityType(section), 
		STG_TOKEN_MAX );
	strncpy(child.color, wf.ReadString(section,"color","red"), 
		STG_TOKEN_MAX);
	child.parent_id = parent; // make a new entity on the root 

	// decode the token into a type number
	if( strcmp( "position", child.token ) == 0 )
	  child.type = STG_MODEL_POSITION;
	else
	  child.type = STG_MODEL_GENERIC;

	// warn the user if no name was specified
	if( strcmp( child.name, "" ) == 0 )
	  PLAYER_MSG2( "stage1p4: model %s (line %d) has no name specified. Player will not be able to access this device", child.token, line );
	
	stg_id_t banana = stg_model_create( cli, &child );
	
	// associate the name 
	strncpy( created_models[section].name, child.name, STG_TOKEN_MAX );
	
	PLAYER_MSG3( "stage1p4: associating section %d name %s "
		     "with stage model %d",
		     section, 
		     created_models[section].name, 
		     created_models[section].stage_id );
	
	// remember the model id for this section
	created_models[section].stage_id = banana;
	
	PLAYER_MSG1( "created model %d", banana );

	// TODO - is there a nicer way to handle unspecified settings?
	// right now i set stupid defaults and test for them

	stg_size_t sz;
	sz.x = wf.ReadTupleFloat( section, "size", 0, -99.0 );
	sz.y = wf.ReadTupleFloat( section, "size", 1, -99.0 );

	if( sz.x != -99 && sz.y != 99 )
	  stg_model_set_size( cli, banana, &sz );
	
	stg_velocity_t vel;
	vel.x = wf.ReadTupleFloat( section, "velocity", 0, 0.0 );
	vel.y = wf.ReadTupleFloat( section, "velocity", 1, 0.0 );
	vel.a = wf.ReadTupleFloat( section, "velocity", 2, 0.0 );
	stg_model_set_velocity( cli, banana, &vel );
	
	stg_pose_t pose;
	pose.x = wf.ReadTupleFloat( section, "pose", 0, 0.0 );
	pose.y = wf.ReadTupleFloat( section, "pose", 1, 0.0 );
	pose.a = wf.ReadTupleFloat( section, "pose", 2, 0.0 );
	stg_model_set_pose( cli, banana, &pose );

	const char* bitmapfile = wf.ReadString(section,"bitmap", "" );
	if( strcmp( bitmapfile, "" ) != 0 )
	  {
	    PLAYER_MSG1("Loading bitmap file \"%s\"", bitmapfile );

	    FILE *bitmap = fopen(bitmapfile, "r" );
	    if( bitmap == NULL )
	      {
		PLAYER_WARN1("failed to open bitmap file \"%s\"", bitmapfile);
		perror( "fopen error" );
	      }
	    
	    struct pam inpam;
	    pnm_readpaminit(bitmap, &inpam, sizeof(inpam)); 
	
	    printf( "read image %dx%dx%d",
		    inpam.width, 
		    inpam.height, 
		    inpam.depth );
	    // convert the bitmap to rects and poke them into the model
	  }

      } 
  }
  return cli;
}


int Stage1p4::DestroyStageClient( stg_client_t* cli )
{ 
  PLAYER_MSG0( "STAGE DRIVER DESTROY CLIENT" );
  stg_client_free( cli );
  return 0;
}


int 
Stage1p4::Setup()
{
  PLAYER_MSG0( "STAGE DRIVER SETUP" );
  return 0;
}

int 
Stage1p4::Shutdown()
{
  PLAYER_MSG0( "STAGE DRIVER SHUTDOWN" );
  return 0;
}

// END BASE CLASS

