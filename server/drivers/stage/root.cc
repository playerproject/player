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

#define ROOTDEF {PLAYER_STAGE_CREATE_MODEL, "root", "root", 0, -1, 0.0, 0.0, 0.9}    

player_stage_model_t root_model = { PLAYER_STAGE_CREATE_MODEL, 
				    "root", 
				    "root", 
				    0, 
				    -1, 
				    0.0, 0.0, 0.9 };    

// constructor
CRootEntity::CRootEntity()
  : CEntity( &root_model )    
{
  PRINT_DEBUG( "Creating root model" );
  
  CEntity::root = this; // phear me!

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


