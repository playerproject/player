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
 * Desc: Simulates a sonar ring.
 * Author: Andrew Howard, Richard Vaughan
 * Date: 28 Nov 2000
 * CVS info: $Id$
 */

#include <assert.h>
#include <math.h>
#include "stg_sonar.hh"
#include "raytrace.hh"

// constructor
CSonarModel::CSonarModel( char* name, char* type, 
			  char* color, CEntity *parent )
  : CEntity( name, type, color, parent )    
{
  this->min_range = 0.20;
  this->max_range = 5.0;

  this->m_interval = 0.1; // 10Hz update
  
  
  // we don't need a body rectangle 
  SetRects( NULL, 0 );
  
  // by default we inherit the size of our parent
  if( m_parent_entity )
    {
      this->size_x = m_parent_entity->size_x;
      this->size_y = m_parent_entity->size_y;
    }
  
  // Initialise the sonar poses to default values
  this->transducer_count = 16; // a sensible default
  
  // arrange the transducers in a circle around the perimeter
  for (int i = 0; i < this->transducer_count; i++)
    {
      double angle = i * TWOPI / this->transducer_count;
      this->transducers[i][0] = this->size_x/2.0 * cos( angle );
      this->transducers[i][1] = this->size_y/2.0 * sin( angle );
      this->transducers[i][2] = angle;
    }  
}

///////////////////////////////////////////////////////////////////////////
// Update the sonar data
int CSonarModel::Update() 
{ 
  //PRINT_WARN( "update" );

  CEntity::Update();
  
  // is anyone interested in my data? if not, bail here.
  if( !IsSubscribed( STG_PROP_ENTITY_DATA ) )
    return 0;

  //PRINT_WARN( "subscribed" );
  
  //PRINT_WARN3( "time now %.4f time since last update %.4f interval %.4f",
  //       CEntity::simtime,  CEntity::simtime - m_last_update, m_interval );

  // Check to see if it is time to update
  //  - if not, return right away.
  if( CEntity::simtime - m_last_update < m_interval)
    return 0;

  //PRINT_WARN( "time to upate" );

  m_last_update = CEntity::simtime;
  
  // place to store the ranges as we generate them
  //double* ranges = new double[transducer_count];
  double ranges[transducer_count];
  
  if( this->power_on )
    {
      //PRINT_WARN( "power on" );
      
      // Do each sonar
      for (int s = 0; s < this->transducer_count; s++)
	{
	  // Compute parameters of scan line
	  double ox = this->transducers[s][0];
	  double oy = this->transducers[s][1];
	  double oth = this->transducers[s][2];
	  LocalToGlobal(ox, oy, oth);
	  
	  ranges[s] = this->max_range; // max threshold
	  
	  CLineIterator lit( ox, oy, oth, this->max_range, 
			     CEntity::matrix, PointToBearingRange );
	  CEntity* ent;
	  
	  while( (ent = lit.GetNextEntity()) )
	    {
	      if( ent != this && ent != m_parent_entity && ent->sonar_return ) 
		{
		  ranges[s] = lit.GetRange();

		  // min threshold
		  if( ranges[s] < this->min_range ) 
		    ranges[s] = this->min_range;
		  break;
		}
	    }	
	}
    }
  
  //PRINT_WARN1( "sonar exporting data at %.4f seconds", CEntity::simtime );
  
  // this is the right way to export data, so everyone else finds out about it
  Property( -1, STG_PROP_ENTITY_DATA, 
	    (char*)ranges, transducer_count * sizeof(ranges[0]), NULL );
  
  return 0;
}


#ifdef INCLUDE_RTK2

///////////////////////////////////////////////////////////////////////////
// Initialise the rtk gui
int CSonarModel::RtkStartup(rtk_canvas_t* canvas)
{
  if( CEntity::RtkStartup(canvas) == -1 )
    {
      PRINT_ERR1( "model %s (sonar) base startup failed", this->name );
      return -1;
    }
  
  // there still might not be a canvas
  if( canvas )
    {
      // Create a figure representing this object
      this->scan_fig = rtk_fig_create( canvas, this->fig, 49);
      assert( scan_fig );

      // Set the color
      rtk_fig_color_rgb32(this->scan_fig, 0xCCCCCC );
    }

  return 0; //success
}


///////////////////////////////////////////////////////////////////////////
// Finalise the rtk gui
void CSonarModel::RtkShutdown()
{
  // Clean up the figure we created
  if( this->scan_fig) 
    {
      rtk_fig_destroy(this->scan_fig);
      this->scan_fig = NULL;
    }
  
  CEntity::RtkShutdown();
} 


///////////////////////////////////////////////////////////////////////////
// Update the rtk gui
int CSonarModel::RtkUpdate()
{
  if( CEntity::RtkUpdate() == -1 )
    {
      PRINT_ERR1( "model %s (sonar) base update failed", this->name );
      return -1;
    } 
  if( this->scan_fig )
    {
      rtk_fig_clear(this->scan_fig);
 
      double* ranges = new double[transducer_count];
      size_t len = transducer_count * sizeof(ranges[0]);
      size_t gotlen = len;

      // if subscribed and data is available, render it
      if( IsSubscribed( STG_PROP_ENTITY_DATA ) && 
	  //ShowDeviceData( this->lib_entry->type_num) &&
	  this->GetData( (char*)ranges, &gotlen ) == 0 && 
	  gotlen == len ) 
	{	    
	  for (int s = 0; s < transducer_count; s++)
	    {
	      // draw an arrow from the transducer to it's hit point
	      rtk_fig_arrow(this->scan_fig, 
			    this->transducers[s][0], 
			    this->transducers[s][1], 
			    this->transducers[s][2],
			    ranges[s], 0.05 );
	      
	    }
	}
    }
  return 0; //success
}

#endif
