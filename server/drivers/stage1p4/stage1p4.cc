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
char* Stage1p4::world_file = STG_DEFAULT_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;
ConfigFile* Stage1p4::config = NULL;
CWorldFile Stage1p4::wf;

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
      
      // now look up the Stage worldfile section number for this device
      // (note - this is NOT our Player config file section number)

      this->model = NULL;

      if( this->stage_client )
	{	  
	  for( int i=1; i<this->stage_client->model_count; i++ )
	    if( strcmp( model_name, this->stage_client->models[i]->name) == 0)
	      {
		this->model =this->stage_client->models[i];
		break;
	      }
	}
      
      if( this->model  == NULL )
	PLAYER_ERROR1( "device %s can't find a Stage model with the same name",
		       model_name );
    }
}


int Stage1p4::Setup()
{ 
  stg_subscription_t sub;
  sub.id = this->model->id;
  sub.prop = this->subscribe_prop;
  sub.status = STG_SUB_SUBSCRIBED;

  stg_fd_msg_write( this->stage_client->pollfd.fd, 
		    STG_MSG_SUBSCRIBE, &sub, sizeof(sub) );
  
  return 0;
};

int Stage1p4::Shutdown()
{ 
  stg_subscription_t sub;
  sub.id = this->model->id;
  sub.prop = this->subscribe_prop;
  sub.status = STG_SUB_UNSUBSCRIBED;

  stg_fd_msg_write( this->stage_client->pollfd.fd, 
		    STG_MSG_SUBSCRIBE, &sub, sizeof(sub) );
  
  return 0;
};


// destructor
Stage1p4::~Stage1p4()
{
  if( this->stage_client )
    stg_client_free( this->stage_client );
}

// END CLASS
  
  
