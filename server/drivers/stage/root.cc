/*
 *  Stage : a multi-robot simulator.
 *  Copyright (C) 2001, 2002 Richard Vaughan, Andrew Howard and Brian Gerkey.
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
/*
 * Desc: A root device model - replaces the CWorld class
 * Author: Richard Vaughan
 * Date: 31 Jan 2003
 * CVS info: $Id$
 */

#define DEBUG

#include <math.h>
#include <stdio.h>
#include <assert.h>

#include "stage.h"
#include "root.hh"
#include "matrix.hh"

#ifdef INCLUDE_RTK2
#include "device.h" // from (toplevel)/server
#include "rtkgui.hh"
#endif

#define PLAYER_STAGE_ROOT_NAME "root"

// constructor
CRootEntity::CRootEntity( const stage_libitem_t items[] )
  : CEntity( PLAYER_STAGE_ROOT_NAME, "root", "red", NULL )    
{
  PRINT_DEBUG( "Creating root model" );
  
  CEntity::root = this; // phear me!

  // init the storage for created models
  // the hash keys will be char strings
  //ents =  g_hash_table_new( g_str_hash, g_str_equal );
 
  // the hash keys will be integers
  ents =  g_hash_table_new( g_int_hash, g_int_equal() );
  
  // add myself to the hash table
  
  g_hash_table_insert( ents, 0, this );

  size_x = 10.0; // a 10m world by default
  size_y = 10.0;
  ppm = 10; // default 10cm resolution passed into matrix (DEBUG)
  //this->ppm = 100; // default 10cm resolution passed into matrix

  // the global origin is the bottom left corner of the root object
  origin_x = size_x/2.0;
  origin_y = size_y/2.0;

  vision_return = false; 
  laser_return = LaserVisible;
  sonar_return = true;
  obstacle_return = true;
  idar_return = IDARReflect;
  
  /// default 5cm resolution passed into matrix
  double ppm = 20.0;
  PRINT_DEBUG3( "Creating a matrix [%.2fx%.2f]m at %.2f ppm",
		size_x, size_y, ppm );
  
  assert( matrix = new CMatrix( size_x, size_y, ppm, 1) );
  
  this->libitems = items;
  
  // count all the items in the library array
  libitems_count = 0;
  for( stage_libitem_t* item = (stage_libitem_t*)items; item->token; item++ )
    {
      PRINT_DEBUG4( "counting library item %d : %s %s %p", 
		    libitems_count, item->token, item->colorstr, item->fp );
      
      libitems_count++;
    }
  
  stage_gui_config_t default_gui;
  strncpy( default_gui.token, STG_DEFAULT_GUI, STG_TOKEN_MAX);
  default_gui.width = 300;
  default_gui.height = 300;
  default_gui.ppm = 20.0;
  default_gui.originx = default_gui.width/2.0;
  default_gui.originy = default_gui.height/2.0;
  default_gui.showgrid = true;
  default_gui.showdata = true;
  default_gui.showsubscribedonly = true;

#ifdef INCLUDE_RTK2
  grid_enable = true;
#endif
}

int CRootEntity::Property( int con, stage_prop_id_t property, 
			      void* value, size_t len, stage_buffer_t* reply )
{
  PRINT_DEBUG3( "setting prop %d (%d bytes) for ROOT ent %p",
		property, len, this );
    

  switch( property )
    {      
      // we'll tackle this one, 'cos it means creating a new matrix
    case STG_PROP_ENTITY_SIZE:
      PRINT_WARN( "setting size of ROOT" );
      assert( len == sizeof( stage_size_t ) );
      matrix->Resize( size_x, size_y, ppm );      
      this->MapFamily();
      break;
      
      /*    case STG_PROP_ROOT_PPM:
      PRINT_WARN( "setting PPM of ROOT" );
      assert(len == sizeof(ppm) );
      memcpy( &(ppm), value, sizeof(ppm) );
      
      matrix->Resize( size_x, size_y, ppm );      
      this->MapFamily();
      break;
      */

    case STG_PROP_ROOT_PPM:
      if( value )
	{
	  PRINT_WARN( "setting PPM" );
	  assert(len == sizeof(double) );
	  
	  double ppm = *((double*)value);
	  
	  if( CEntity::matrix )
	    {
	      CEntity::matrix->Resize( size_x, size_y, ppm );      
	      this->MapFamily();	  
	    }
	  else
	    PRINT_WARN( "trying to set ppm for non-existent matrix" );
	}
      
      // if a reply is needed, add the current PPM to the buffer
      if( reply )
	{
	  double ppm = -1;
	  
	  if( CEntity::matrix )
	    ppm = CEntity::matrix->ppm;
	  
	  // SIOBufferProperty( reply, this->stage_id, STG_PROP_ROOT_PPM,
	  //	     &ppm, sizeof(ppm), STG_ISREPLY );
	}
      break;
      
    default: 
      break;
    }

  // get the inherited behavior 
  return CEntity::Property( con, property, value, len, reply  );
}



// look in the array for an item with matching token and return a pointer to it
stage_libitem_t* CRootEntity::FindItemWithToken( const stage_libitem_t* items, 
					     int count, char* token )
{
  for( int c=0; c<count; c++ )
    if( strcmp( items[c].token, token ) == 0 )
      return (stage_libitem_t*)&(items[c]);
  
  return NULL; // didn't find the token
}

// destroy all my children and their descendents
int CRootEntity::DestroyAll() 
{
  CHILDLOOP(ch)
    DestroyModel( ch->name );

  return 0;
}


int CRootEntity::DestroyModel( char* name )
{
  
  // look up the pointer
  CEntity* ent = (CEntity*)g_hash_table_lookup( ents, (gpointer)name );
  PLAYER_TRACE2("destroying model %s (%p)", name, ent );
  
  if( ent ) // if it's no good, don't kill it.
    {
      // recursively destroy its children
      for( CEntity* ch = ent->child_list; ch; ch = ch->next )
	DestroyModel( ch->name );
      
      ent->Shutdown();
      ent->RtkShutdown();
      
      // remove this entity from it's parent's childlist
      STAGE_LIST_REMOVE( ent->m_parent_entity->child_list, ent)
	
      // forget about it
      g_hash_table_remove( ents, (gpointer)name );

      delete ent;
    }

  return 0;
}

// create an instance of an entity given a worldfile token
int CRootEntity::CreateModel( player_stage_model_t* model )
{
  assert( model );

  static int model_count = 0;

  // look up this token in the library
  stage_libitem_t* libit = FindItemWithToken( model->type );
  
  CEntity* ent;
  
  if( libit == NULL )
    {
      PRINT_ERR1( "requested model type '%s' is not in the library", 
		  model->type );
      return -1;
    }
  
  if( libit->fp == NULL )
  {
    PRINT_ERR1( "requested model type '%s' doesn't have a creator function", 
		model->type );
    return -1;
  }
  
  
  PRINT_DEBUG3( "creating a %s model with name \"%s\" with parent \"%s\"",
		model->type, model->name, model->parent );
  
  // if it has a valid parent, look up the parent's address
  CEntity* parentPtr = GetEnt( model->parent);

  if( !parentPtr )
    {
      PRINT_ERR1( "No model exists with name \"%s\"",
		  model->parent );
      return -1;
    }
  
   // create an entity through the creator callback fucntion  
   // which calls the constructor
   ent = (*libit->fp)( model->name, 
		       model->type, 
		       (char*)libit->colorstr, 
		       parentPtr ); 

   assert( ent );

   // set the pose of the new entity
   ent->SetPose( model->px, model->py, model->pa );

   // associate this id with this entity

   //int* key = (int*)g_new(int, 1);
   //*key = model->id;
   g_hash_table_insert( ents, model->id, ent);
   PRINT_WARN1( "inserting model %d name \"%s\" into hash table", 
		model->id, model->name );

   // need to do some startup outside the constructor to allow for
   // full polymorphism
   if( ent->Startup() == -1 ) // if startup fails
     {
       PRINT_WARN3( "Startup failed for model %s:%s at %p",
		    model->name, model->type, ent );
       delete ent;
       return -1;
     }

   // now start the GUI repn for this entity
   if( ent->RtkStartup(this->canvas) == -1 )
     {
       PRINT_WARN3( "Gui startup failed for model %s:%s at %p",
		    model->name, model->type, ent );
       delete ent; // destructor calls CEntity::Shutdown()
       return -1;
     }
        

     if( ent ) PRINT_DEBUG3(  "Startup successful for model %s:%s at %p",
			    model->name, model->type, ent );
     return 0; //success
 } 

 void CRootEntity::Print( void )
 {
   PRINT_MSG("[Library contents:");

   int i;
   for( i=0; i<libitems_count; i++ )
     printf( "\n\t%s:%s:%p", 
	     libitems[i].token, libitems[i].colorstr, libitems[i].fp );
   
   puts( "]" );
 }

