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

// pointer to a function that returns a new  entity
typedef CEntity*(*CreatorFunctionPtr)( char* name, char* type, 
				       char* color, CEntity *parent );

typedef CreatorFunctionPtr CFP; // abbreviation

// associate a world file token with a model creator function (and a color).
// an array of these is passed to root's constructor
typedef struct 
{
  const char* token;
  const char* colorstr;
  CreatorFunctionPtr fp;
} stage_libitem_t;


class CRootEntity : public CEntity
{
  // Constructor takes a database of model types
public: CRootEntity(  const stage_libitem_t items[] );
  
  // Startup routine
private: int CreateMatrix( void );

  double ppm; // passed into CMatrix creator


private:
  // an array of [token,color,creator_function] structures
  // the position in the array is used as a type number
  const stage_libitem_t* libitems;
  int libitems_count;

  // associate model instances with an id number
  GHashTable* ents;

  
public:
  
  int NumModels(void)
  { return( g_hash_table_size( this->ents ) ); };
  
  // print the items on stdout
  void Print(void);
  
  // returns a pointer the matching item, or NULL if none is found
  stage_libitem_t* FindItemWithToken( const stage_libitem_t* items, 
				      int count, char* token );
  
  // as above, but uses data members as arguments
  stage_libitem_t* FindItemWithToken( char* token  )
  { return FindItemWithToken( this->libitems, this->libitems_count, token ); };
  
  void AddDevice( const char* token, 
		  const char* colorstr, 
		  CreatorFunctionPtr creator );
  
  // create an instance of an entity given a request returns zero on success.
  int CreateModel( player_stage_model_t* model );
  
  // remove an entity and its children from the sim
  int DestroyModel( char* name );

  // destroy all my children and their descendents
  int DestroyAll();

  CEntity* GetEnt( char* name )
  {
    return( (CEntity*)g_hash_table_lookup( ents, (gpointer)name ) ); 
  };

  // Update the device
public: 
  //  virtual void Update(double sim_time);
  virtual int Property( int con, stage_prop_id_t property, 
			void* value, size_t len, stage_buffer_t* reply );
  
};

#endif

