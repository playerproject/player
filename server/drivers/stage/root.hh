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
 * Desc: Simulates a root ring.
 * Author: Andrew Howard, Richard Vaughan
 * Date: 28 Nov 2000
 * CVS info: $Id$
 */

#ifndef ROOTDEVICE_HH
#define ROOTDEVICE_HH

#include <glib.h>

#include "entity.hh"

class CRootEntity : public CEntity
{
  // Constructor takes a database of model types
public: CRootEntity( void );
  
  // Startup routine
private: int CreateMatrix( void );

  double ppm; // passed into CMatrix creator
  
public:
  
  int NumModels(void)
  { return( g_hash_table_size( this->ents ) ); };
  
  // print the items on stdout
  void Print(void);
  
  void AddDevice( const char* token, 
		  const char* colorstr, 
		  CreatorFunctionPtr creator );
  
  // create an instance of an entity given a request returns zero on success.
  int CreateModel( player_stage_model_t* model );
  
  // remove an entity and its children from the sim
  int DestroyModel();

  // destroy all my children and their descendents
  int DestroyAll();

  // Update the device
public: 
  //  virtual void Update(double sim_time);
  virtual int Property( int con, stage_prop_id_t property, 
			void* value, size_t len, stage_buffer_t* reply );
  
};

#endif

