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

#include "stage1p4.h"
#include "stg_time.h"

// init static vars
char Stage1p4::worldfile_name[MAXPATHLEN] = STG_DEFAULT_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;
ConfigFile* Stage1p4::config = NULL;
CWorldFile Stage1p4::wf;
char* Stage1p4::world_name;
pthread_mutex_t Stage1p4::model_mutex;
pthread_mutex_t Stage1p4::reply_mutex;
bool Stage1p4::init = true;

// declare Player's emergency stop function (defined in main.cc)
void Interrupt( int dummy );
				   
// constructor
//
Stage1p4::Stage1p4(char* interface, ConfigFile* cf, int section, 
		   size_t datasz, size_t cmdsz, int rqlen, int rplen) :
  CDevice(datasz,cmdsz,rqlen,rplen)
{
  PLAYER_TRACE1( "Stage1p4 device created for interface %s\n", interface );
  
  this->config = cf;
    
  const char *enttype = config->GetEntityType(section);

  
  if( this->init )
    {
      PLAYER_TRACE0( "Initializing Stage1p4 driver" );
      assert( pthread_mutex_init(&model_mutex,NULL) == 0 );
      assert( pthread_mutex_init(&reply_mutex,NULL) == 0 );
      this->init = false;
    }
 
  //printf( "processing entity type %s\n", enttype );
  
  if( strcmp( enttype, "simulation" ) == 0 )
    {
      //printf( "ignoring simulation device" );
    }
  else
    {
      char* model_name = 
	(char*)config->ReadString(section, "model", 0 );
      
      if( model_name == NULL )
	PLAYER_ERROR1( "device \"%s\" uses the Stage1p4 driver but has "
		       "no \"model\" value defined.",
		       model_name );
      
      PLAYER_TRACE1( "attempting to resolve Stage model \"%s\"", model_name );
      
      this->model = stg_client_get_model( Stage1p4::stage_client,
					  Stage1p4::world_name, 
					  model_name );
      if( this->model  == NULL )
	PLAYER_ERROR1( "device %s can't find a Stage model with the same name",
		       model_name );
    }
}


int Stage1p4::Setup()
{ 
  if( this->model && this->subscribe_prop )
    {
      puts( "SUBSCRIBING" );
      stg_model_subscribe( this->model, this->subscribe_prop, 0.1 );  
    }
  return 0;
};

int Stage1p4::Shutdown()
{ 
  if( this->model && this->subscribe_prop )
    {
      puts( "UNSUBSCRIBING" );
      stg_model_unsubscribe( this->model, this->subscribe_prop );  
    }
  return 0;
};


// destructor
Stage1p4::~Stage1p4()
{
  if( this->stage_client )
 {
   stg_client_destroy( this->stage_client );
   this->stage_client = NULL;
 }
}


void Stage1p4::Update()
{
  assert( Stage1p4::stage_client );
  
  stg_msg_t* msg = NULL;      
 
  //putchar( 'u' );

  // handle any packets coming in from Stage
  while( (msg = stg_client_read( Stage1p4::stage_client )) )
    {
      //puts( "!" );
      
      stg_client_handle_message( Stage1p4::stage_client, msg );
      free( msg );
    }	
  
  //  unblock any devices that were waiting for this one.
  this->DataAvailable();
}    


// END CLASS
  
  
