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

#define PLAYER_ENABLE_TRACE 0
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
char* Stage1p4::world_file = DEFAULT_STG_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;
stg_model_t* Stage1p4::models = NULL;
int Stage1p4::models_count = 0;
ConfigFile* Stage1p4::config = NULL;
CWorldFile Stage1p4::wf;

double Stage1p4::time = 0.0;
int Stage1p4::stage_port = 6601;
char* Stage1p4::stage_host = "localhost";
stg_property_t* Stage1p4::stage_time = NULL;

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
      this->section = 0;   
      for( int i=1; i<models_count; i++ )
	if( strcmp( model_name, models[i].name ) == 0 )
	  {
	    this->section = i;
	    break;
	  }
      
      if( this->section == 0 )
	PLAYER_ERROR1( "device %s can't find a Stage model with the same name",
		       model_name );
    }
}

// destructor
Stage1p4::~Stage1p4()
{
  if( this->stage_client )
    stg_client_free( this->stage_client );
}

void Stage1p4::StageSubscribe( stg_prop_id_t data )
{ 
  stg_model_t* model = &Stage1p4::models[this->section];

  PLAYER_TRACE2( "stage1p4 starting device (%d:%s)", 
		 model->stage_id, model->name );
  
  // subscribe to our data
  stg_property_t* prop =  stg_property_create();
  prop->id = model->stage_id;
  prop->timestamp = 1.0;
  prop->property = data;
  prop->action = STG_SUBSCRIBE;
  stg_property_write( Stage1p4::stage_client, prop );
  stg_property_free( prop );
  
  // wait until a subscription reply comes back
  int timeout = 500;
  while( model->subs[data] == 0 )
    {
      this->CheckForData();
      
      if( --timeout == 0 )
	break;
      usleep(100);
    }

  switch( model->subs[data] )
    {
    case 0:
      PRINT_ERR2( "stage1p4: subscription (%d:%s) failed (timeout)",
		     prop->id, stg_property_string(prop->property) );
      exit(-1);
      break;
      
    case -1:
      PRINT_ERR2( "stage1p4: subscription (%d:%s) failed (no such model)",
		     prop->id, stg_property_string(prop->property) );
      exit(-1);
      break;
      
    case 1:
      PLAYER_TRACE2( "stage1p4: subscription (%d:%s) succeeded",
		   prop->id, stg_property_string(prop->property) );
      break;
      
    default:
      PRINT_ERR3( "stage1p4: subscription (%d:%s) failed (code %d)",
		   prop->id, stg_property_string(prop->property),  
		   model->subs[data] );      
      break;
    }
}


void Stage1p4::StageUnsubscribe( stg_prop_id_t data )
{ 
  stg_model_t* model = &Stage1p4::models[this->section];
  
  PLAYER_TRACE2( "stage1p4 stopping device (%d:%s)", 
	       model->stage_id, model->name );
  
  // subscribe to our data
  stg_property_t* prop =  stg_property_create();
  prop->id = model->stage_id;
  prop->timestamp = 1.0;
  prop->property = data;
  prop->action = STG_UNSUBSCRIBE;
  stg_property_write( Stage1p4::stage_client, prop );
  stg_property_free( prop );
  
  // wait until a subscription reply comes back
  int timeout = 1000;
  while( model->subs[data] == 1 )
    {
      this->CheckForData();
      
      if( --timeout == 0 )
	break;
      usleep(100);
    }

  switch( model->subs[data] )
    {
    case 1:
      PRINT_ERR2( "stage1p4: unsubscription (%d:%s) failed (timeout)",
		     prop->id, stg_property_string(prop->property) );
      exit(-1);
      break;
      
    case -1:
      PRINT_ERR2( "stage1p4: subscription (%d:%s) failed (no such model)",
		     prop->id, stg_property_string(prop->property) );
      exit(-1);
      break;
      
    case 0:
      PLAYER_TRACE2( "stage1p4: unsubscription (%d:%s) succeeded",
		   prop->id, stg_property_string(prop->property) );
      break;
      
    default:
      PRINT_ERR3( "stage1p4: unsubscription (%d:%s) failed (code %d)",
		   prop->id, stg_property_string(prop->property),  
		   model->subs[data] );      
      break;
    }
}

void Stage1p4::WaitForData( stg_id_t model, stg_prop_id_t datatype )
{
  // wait until a property of this type shows up in the buffer
  while( Stage1p4::models[this->section].props[datatype] == 0 )
    {
      printf( "waiting for a property for %s\n", 
	      stg_property_string(datatype) );
      
      this->CheckForData();
      puts("sleeping");
      usleep(100);
    }
}

void Stage1p4::CheckForData( void )
{
  // see if any data is pending
  poll( &Stage1p4::stage_client->pollfd, 1, 0 );
  
  if( Stage1p4::stage_client->pollfd.revents & POLLIN )
    {
      //puts( "POLLIN" );
      stg_property_t* prop = stg_property_read( Stage1p4::stage_client );

      // TODO - this doesn't seem to b triggered when Stage dies. what
      // is up with that?
      if( prop == NULL )
	{
	  printf( "Stage1p4: failed to read from Stage. Quitting.\n" );
	  Interrupt(0); // kills Player dead.
	}
     
      //printf( "received property [%d:%s]\n",
      //      prop->id, stg_property_string(prop->property) );

      // if this is a time packet, we stash it in the static clock
      if( prop->property == STG_WORLD_TIME )
	{ 
	  // TODO - assume stage_time is a fixed size to get rid of
	  // this realloc
	  // printf( "time packet! (%.3f)\n", *(double*)prop->data );
	  stage_time =  (stg_property_t*)
	    realloc( stage_time, sizeof(stg_property_t) + prop->len );
	  memcpy( stage_time, prop, sizeof(stg_property_t) + prop->len );
	}
      else if( prop->property == STG_WORLD_SAVE )
	{
	  puts( "PLAYER SAVE" );

	  // request fresh pose data for everything
	  // Iterate through sections, requesting pose data for each entity
	  for (int section = 1; section < models_count; section++)
	    {
	      if( strcmp( "gui", wf.GetEntityType(section) ) == 0 )
		{
		  //PRINT_WARN( "save gui section not implemented" );
		}
	      else
		{
		  stg_id_t anid = models[section].stage_id;
		  
		  char* name = models[section].name;
		  PLAYER_TRACE3( "requesting pose data for model %d \"%s\" "
				 "section %d\n", 
				 anid,  models[section].name, section );

		  // zap any old pose data
		  free(models[section].props[STG_MOD_POSE]);
		  models[section].props[STG_MOD_POSE] = NULL;
		  
		  // ask for new pose data		  
		  stg_property_t* prop = stg_property_create(); 
		  prop->id = anid;
		  prop->action = STG_GET;
		  prop->property = STG_MOD_POSE;
		  
		  stg_property_write( Stage1p4::stage_client, prop );
		  free( prop );

		  // wait for the pose data to show up
		  while( models[section].props[STG_MOD_POSE] == NULL )
		    {
		      PLAYER_TRACE4( "waiting for [%d:%s] data to show up (section %d) at %p",
				     anid, 
				     stg_property_string(STG_MOD_POSE),
				     section,
				     models[section].props[STG_MOD_POSE] );
		      
		      CheckForData();
		      usleep(1);
		    }
		}
	    }
	  
	  Stage1p4::wf.DownloadAndSave( Stage1p4::stage_client, 
					Stage1p4::models,
					Stage1p4::models_count );
	}
      else
	{	
	  // find the incoming stage id in the model array and poke in the
	  // data. TODO - use a hash table instead of an array so we
	  // avoid this search
	  stg_model_t* target = NULL;
	  
	  for( int i=0; i<Stage1p4::models_count; i++ )
	    {
	      if( Stage1p4::models[i].stage_id == prop->id )
		{
		  target = &Stage1p4::models[i];
		  //printf( " - target is section %d\n", i );
		  break;
		}
	    }
	  
	  if( target == NULL )
	    PRINT_ERR1( "stage1p4: received property for unknown model (%d)",
			prop->id );
	  else
	    {
	      // handle subscription results
	      if( prop->action == STG_SUBSCRIBE || 
		  prop->action == STG_UNSUBSCRIBE )
		{
		  PLAYER_TRACE3( "subscription reply! [%d:%s] - %d\n",
				prop->id, 
				stg_property_string(prop->property),
				*(char*)prop->data );
		  
		  target->subs[prop->property] = *prop->data;
		}
	      else // everything else gets stored in the property cache
		{		  
		  //printf( "storing [%d:%s] %d bytes of data\n",
		  //prop->id, stg_property_string(prop->property), prop->len);
		  
		  // stash this property in the device's array of pointers
		  target->props[prop->property] = (stg_property_t*)
		    realloc( target->props[prop->property], 
			     sizeof(stg_property_t) + prop->len );
		  
		  memcpy( target->props[prop->property], prop, 
			  sizeof(stg_property_t) + prop->len );
		  
		  stg_property_free(prop);
		}
	    }
	}
    }
}
// END CLASS
  
  
