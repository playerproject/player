/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
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
 */

/* Desc:Interface to the Stage simulation
 * Authors: Richard Vaughan
 * Created: 20030424
 * $Id$
 */

//#define DEBUG

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>


void StageProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  PLAYER_WARN( "FillData" );

  if(hdr.size != sizeof(player_stage_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of stage data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_stage_data_t),hdr.size);
  }

  this->model_count = (int)ntohl(((player_stage_data_t*)buffer)->model_count);
  this->interval_ms = (int)ntohl(((player_stage_data_t*)buffer)->interval_ms);
}

// interface that all proxies SHOULD provide
void StageProxy::Print()
{
  printf("#STAGE DEVICE (%d:%d:%d) - %c\n", m_device_id.robot, 
         m_device_id.code, m_device_id.index, access);
  printf("model count: %d\n", model_count );
  printf("interval: %d ms\n", interval_ms );
}

int StageProxy::DestroyModel( int model_id )
{
  player_stage_model_t model;
  
  model.subtype = PLAYER_STAGE_DESTROY_MODEL;
  model.id = model_id;
  
  return(client->Request(m_device_id, 
			 (const char*)&model,
                         sizeof(model))); 
}

int StageProxy::DestroyAllModels( void )
{
  uint8_t config = PLAYER_STAGE_DESTROY_ALL;
  
  return(client->Request(m_device_id, 
			 (const char*)&config,
                         sizeof(config))); 
}


int StageProxy::CreateModel( const char* type, 
			     const char* name, 
			     int parent_id,
			     double x, double y, double a )
{
  if(!client)
    return(-1);
  
  // idiot check
  if(parent_id<0) { printf( "error: parent = %d\n", parent_id ); return(-1); }
  if(!name) { printf( "error: NULL name string\n" ); return(-1); }
  if(!type) { printf( "error: NULL type string\n" ); return(-1); }
  if(strlen(name)<1) { printf( "error: empty name string\n" ); return(-1); }
  if(strlen(type)<1) { printf( "error: empty type string\n" ); return(-1); }
  
  player_stage_model_t model;
  player_msghdr_t hdr;

  model.subtype = PLAYER_STAGE_CREATE_MODEL;

  // set up the model structure
  strncpy(model.type, type, PLAYER_MAX_DEVICE_STRING_LEN );
  strncpy(model.name, name, PLAYER_MAX_DEVICE_STRING_LEN );  
  
  model.parent_id = parent_id;
  
  model.id = 0; // this is filled in by Stage on return

  model.px = x;
  model.py = y;
  model.pa = a;

  if(client->Request(m_device_id,
                     (const char*)&model, sizeof(model),
                     &hdr, (char*)&model, sizeof(model)) < 0)
    return(-1);

  printf( "created model type %s name %s parent %d at (%.2f,%.2f,%.2f)\n",
	  model.type, model.name, model.parent_id, 
	  model.px, model.py, model.pa );	  

  return( model.id );
}
