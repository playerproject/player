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
 * Desc: Base class for every entity.
 * Author: Richard Vaughan, Andrew Howard
 * Date: 7 Dec 2000
 * CVS info: $Id$
 */
#if HAVE_CONFIG_H
  #include <config.h>
#endif


#define DEBUG
//#define VERBOSE
//#define RENDER_INITIAL_BOUNDING_BOXES
//#undef DEBUG
#undef VERBOSE

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <float.h> // for FLT_MAX
#include <sys/mman.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <iostream>
#include <glib.h>

#include "entity.hh"
#include "raytrace.hh"
#include "gui.hh"

// init the static vars shared by all entities
CMatrix* CEntity::matrix = NULL;
bool CEntity::enable_gui = true;
CRootEntity* CEntity::root = NULL;
double CEntity::simtime = 0.0; // start the clock at zero time
double CEntity::timestep = 0.01; // default 10ms update

// hash table for storing id-to-pointer mappings
GHashTable* CEntity::ents = g_hash_table_new( NULL, NULL );

///////////////////////////////////////////////////////////////////////////
// main constructor
// Requires a pointer to the parent and a pointer to the world.

CEntity::CEntity( player_stage_model_t* model )
{
 assert( model );

  PRINT_DEBUG4( "creating a %s model %d:\"%s\" with parent %d",
		model->type, model->id, model->name, model->parent_id );

  this->id = model->id;
  this->m_parent_entity = NULL;
  
  // if a valid parent was specified look up the parent's address
  if( model->parent_id >= 0 && 
      !(this->m_parent_entity = GetEntity( model->parent_id ) ) )
    {
      PRINT_ERR1( "Failed to find a parent. No model exists with id %d",
		  model->parent_id );
      return;
    }
  
  // set my starting pose
  this->SetPose( model->px, model->py, model->pa );

  // record my ID so others can find me by it
  g_hash_table_insert( ents, (gpointer)model->id, this );
  
  PRINT_WARN2( "inserting model %d name \"%s\" into hash table",
	       model->id, model->name );
  
  fig_count = 0;
  
  if( model->name )
    strncpy(this->name, model->name, PLAYER_MAX_DEVICE_STRING_LEN );
  
  if( model->type )
    strncpy(this->type, model->type, PLAYER_MAX_DEVICE_STRING_LEN );
  
  
  // TODO = inherit our parent's color by default
  //if( m_parent_entity )
  //color = m_parent_entity->color;
  //else
  this->color = ::LookupColor( "red" ); 
  
  // init the child list data
  // do this early - ie. before calling any functions that access our children
  this->child_list = this->prev = this->next = NULL;
  
  // Set default pose
  this->local_px = this->local_py = this->local_pth = 0;

  // Set global velocity to stopped
  this->vx = this->vy = this->vth = 0;
  
  // unmoveably MASSIVE! by default
  this->mass = 1000.0;

  // default no-voltage.
  this->volts = -1;    

  // STG_PROP_ENTITY_POWER
  this->power_on = 1;

  // Set the default geometry
  this->size_x = this->size_y = 1.0;
  this->origin_x = this->origin_y = 0;

  // TODO
  m_local = true; 
 
  // zero our IO buffer headers
  memset( &buffer_data, 0, sizeof(buffer_data) );
  memset( &buffer_cmd, 0, sizeof(buffer_data) );

  // initialize our shapes 
  this->rects = NULL;
  this->rect_count = 0;
 
  // by default, all non-root entities have a single rectangle
  //if( m_parent_entity )
    {
      // rectangles - start with a single rect
      // it is automagically scaled to fit the size of the entity
      stage_rotrect_t rect;
      rect.x = 0.0;
      rect.y = 0.0;
      rect.a = 0.0;
      rect.w = 1.0;
      rect.h = 1.0;

      this->SetRects( &rect, 1 );
    }
    
  // the default entity is a colored box
  vision_return = true; 
  laser_return = LaserVisible;
  sonar_return = true;
  obstacle_return = true;
  idar_return = IDARReflect;
  puck_return = false;
  idar_return = IDARReflect;
  fiducial_return = FiducialNone; // not a recognized fiducial
  gripper_return = GripperDisabled;

  // Set the initial mapped pose to a dummy value
  this->map_px = this->map_py = this->map_pth = 0;

  m_dependent_attached = false;

  m_interval = 0.1; // update interval in seconds 
  m_last_update = -FLT_MAX; // initialized 
  
  // init the ptr to GUI-specific data
  this->gui_data = NULL;

  // clear any garbage subscription data
  for( int c=0; c<STG_MAX_CONNECTIONS; c++ )
    DestroyConnection(c);


#ifdef INCLUDE_RTK2
  // Default figures for drawing the entity.
  this->fig = NULL;
  this->fig_label = NULL;
  this->fig_grid = NULL;
  
  grid_major = 1.0;
  grid_minor = 0.2;
  grid_enable = false;

  // By default, we can both translate and rotate the entity.
  this->movemask = RTK_MOVE_TRANS | RTK_MOVE_ROT;
#endif

  // STG_PROP_ENTITY_RANGEBOUNDS
  this->min_range = 0.5;
  this->max_range = 5.0;
  
  // attach to my parent
  if( m_parent_entity )
    m_parent_entity->AddChild( this );

}


///////////////////////////////////////////////////////////////////////////
// Destructor
CEntity::~CEntity()
{
  CHILDLOOP(ch)
    delete ch;

  this->RtkShutdown();
  this->Shutdown();
  
  // remove this entity from it's parent's childlist
  STAGE_LIST_REMOVE( m_parent_entity->child_list, this);

  PLAYER_WARN( "removing ent %d from hash table", id );
  // forget about me
  g_hash_table_remove( ents, (gpointer)id );
}

// destroy all my children and their descendents
int CEntity::DeleteChildren() 
{
  CEntity* ch = this->child_list;
  while( ch )
    {
      CEntity* doomed = ch;
      ch = ch->next;
      delete doomed;
    }

  return 0;
}


void CEntity::AddChild( CEntity* child )
{
  //printf( "appending entity %p to parent %p\n", child, this );
  
  STAGE_LIST_APPEND( this->child_list, child ); 
}

void CEntity::GetBoundingBox( double &xmin, double &ymin,
			      double &xmax, double &ymax )
{
  double x[4];
  double y[4];

  double dx = size_x / 2.0;
  double dy = size_y / 2.0;
  double dummy = 0.0;
  
  x[0] = origin_x + dx;
  y[0] = origin_y + dy;
  this->LocalToGlobal( x[0], y[0], dummy );

  x[1] = origin_x + dx;
  y[1] = origin_y - dy;
  this->LocalToGlobal( x[1], y[1], dummy );

  x[2] = origin_x - dx;
  y[2] = origin_y + dy;
  this->LocalToGlobal( x[2], y[2], dummy );

  x[3] = origin_x - dx;
  y[3] = origin_y - dy;
  this->LocalToGlobal( x[3], y[3], dummy );

  //double ox, oy, oth;
  //GetGlobalPose( ox, oy, oth );
  //printf( "origin: %.2f,%.2f,%.2f \n", ox, oy, oth );
  //printf( "corners: \n" );
  //for( int c=0; c<4; c++ )
  //printf( "\t%.2f,%.2f\n", x[c], y[c] );
  
  // find the smallest values and we're done
  //xmin = xmax = x[0];
  for( int c=0; c<4; c++ )
  {
    if( x[c] < xmin ) xmin = x[c];
    if( x[c] > xmax ) xmax = x[c];
  }

  //ymin = ymax = y[0];
  for( int c=0; c<4; c++ )
  {
    if( y[c] < ymin ) ymin = y[c];
    if( y[c] > ymax ) ymax = y[c];
  } 

  //printf( "before children: %.2f %.2f %.2f %.2f\n",
  //  xmin, ymin, xmax, ymax );

  CHILDLOOP( ch )
    ch->GetBoundingBox(xmin, ymin, xmax, ymax ); 

  //printf( "after children: %.2f %.2f %.2f %.2f\n",
  //  xmin, ymin, xmax, ymax );

}

// this is called very rapidly from the main loop
// it allows the entity to perform some actions between clock increments
// (such handling config requests to increase synchronous IO performance)
int CEntity::Sync()
{ 
  // default - do nothing but call the children
  CHILDLOOP( ch ) 
    if( ch->Sync() == -1 )
      return -1;

  return 0;
};


void CEntity::MapFamily()
{
  Map();
  
  CHILDLOOP( ch )
    ch->MapFamily();
}

void CEntity::UnMapFamily()
{
  UnMap();
  
  CHILDLOOP( ch )
    ch->UnMapFamily();
}


///////////////////////////////////////////////////////////////////////////
// Startup routine
// A virtual function that lets entities do some initialization after
// everything has been loaded.
int CEntity::Startup( void )
{
  PRINT_DEBUG( "entity starting up" );
  
  Map();
  
  int success = 0;

  CHILDLOOP( ch )
    success += ch->Startup();

  if( success < 0 )
    return -1; // fail 
  else
    return 0; // success
}


///////////////////////////////////////////////////////////////////////////
// Shutdown routine
int CEntity::Shutdown()
{
  PRINT_DEBUG( "entity shutting down" );

  UnMap();
  
  int success = 0;
  // recursively shutdown our children
  CHILDLOOP( ch ) 
    success += ch->Shutdown();
  
  if( success < 0 )
    return -1; // fail 
  else
    return 0; // success
}

int CEntity::Move( double vx, double vy, double va, double step )
{
  //fprintf( stderr, "%.2f %.2f %.2f   %.2f %.2f %.2f\n", 
  //   px -1, py -1, pa, odo_px, odo_py, odo_pa );
  
  if( vx == 0 && vy ==0 && va == 0 )
    return 0;
  
  // Compute movement deltas
  // This is a zero-th order approximation
  double dx = step * vx * cos(local_pth) - step * vy * sin(local_pth);
  double dy = step * vx * sin(local_pth) + step * vy * cos(local_pth);
  double da = step * va;
  
  // compute a new pose by shifting us a little from the current pose
  double qx = local_px + dx;
  double qy = local_py + dy;
  double qa = local_pth + da;

  // Check for collisions
  // and accept the new pose if ok
  if (TestCollision(qx, qy, qa ) != NULL)
    {
      SetGlobalVel(0, 0, 0);
      //this->stall = true;
    }
  else
    SetPose(qx, qy, qa);  // set pose takes care of marking us dirty
  
  
  return 0; // success
}
///////////////////////////////////////////////////////////////////////////
// Update the entity's representation
int CEntity::Update()
{
  // recursively update our children
  CHILDLOOP( ch ) 
    if( ch->Update() == -1 )
      return( -1 );
  
  // if we have some speed, move!
  if( vx || vy || vth )
    return Move( vx, vy, vth, CEntity::timestep );
  
  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Render the entity into the world
void CEntity::Map(double px, double py, double pth)
{
  // get the pose in local coords
  this->GlobalToLocal( px, py, pth );

  // add our center of rotation offsets
  px += origin_x;
  py += origin_y;

   // convert back to global coords
  this->LocalToGlobal( px, py, pth );

  this->map_px = px;
  this->map_py = py;
  this->map_pth = pth;


  MapEx(map_px, map_py, map_pth, true);
}

void CEntity::Map( void )
{
  double x,y,a;
  GetGlobalPose( x,y,a );
  Map(x,y,a);
}

///////////////////////////////////////////////////////////////////////////
// Remove the entity from the world
void CEntity::UnMap()
{
  MapEx(this->map_px, this->map_py, this->map_pth, false);
}


///////////////////////////////////////////////////////////////////////////
// Remap ourself if we have moved
void CEntity::ReMap(double px, double py, double pth)
{
  assert( matrix );
  
  // if we haven't moved, do nothing
  if (fabs(px - this->map_px) < 1 / matrix->ppm &&
      fabs(py - this->map_py) < 1 / matrix->ppm &&
      pth == this->map_pth)
    return;
  
  // otherwise erase the old render and draw a new one
  UnMap();
  Map(px, py, pth);
}


///////////////////////////////////////////////////////////////////////////
// Primitive rendering function
void CEntity::MapEx(double px, double py, double pth, bool render)
{
    RenderRects( render );
}

// convert the rotated rectangle into global coords, taking into account
// the entities pose and offset and the rectangle scaling
void CEntity::GetGlobalRect( stage_rotrect_t* dest, stage_rotrect_t* src )
{
  dest->x = ((src->x + src->w/2.0) * size_x) - size_x/2.0 + origin_x;
  dest->y = ((src->y + src->h/2.0) * size_y) - size_y/2.0 + origin_y;
  
  dest->a = src->a;
  dest->w = src->w * size_x;
  dest->h = src->h * size_y;
  
  LocalToGlobal( dest->x, dest->y, dest->a );    
}


///////////////////////////////////////////////////////////////////////////
// Check to see if the given pose will yield a collision with obstacles.
// Returns a pointer to the first entity we are in collision with, and stores
// the location of the hit in hitx,hity (if non-null)
// Returns NULL if not collisions.
// This function is useful for writing position devices.
CEntity *CEntity::TestCollision(double px, double py, double pth, 
				double* hitx, double* hity )
{
  // raytrace along all our rectangles. expensive, but most vehicles
  // will just be a single rect, grippers 3 rects, etc. not too bad.
  
  stage_rotrect_t glob;
  
  int r;
  for( r=0; r<this->rect_count; r++ )
    {
      // find the global coords of this rectangle
      GetGlobalRect( &glob, &(this->rects[r]) );
      
      // trace this rectangle in the matrix
      CRectangleIterator rit( glob.x, glob.y, glob.a, 
			      glob.w, glob.h, matrix );
      
      CEntity* ent;
      while( (ent = rit.GetNextEntity()) )
	{
	  if( ent != this && !IsDescendent(ent) && ent->obstacle_return )
	    {
	      if( hitx || hity ) // if the caller needs to know hit points
		{
		  double x, y;
		  rit.GetPos(x,y); // find the points		  
		  if( hitx ) *hitx = x; // and report them
		  if( hity ) *hity = y;

		}
	      return ent; // we hit this object! stop raytracing
	    }
	}
    }
  return NULL;  // done 
}
  


///////////////////////////////////////////////////////////////////////////
// Convert local to global coords
void CEntity::LocalToGlobal(double &px, double &py, double &pth)
{
  // Get the pose of our origin wrt global cs
  double ox, oy, oth;
  GetGlobalPose(ox, oy, oth);

  // Compute pose based on the parent's pose
  double sx = ox + px * cos(oth) - py * sin(oth);
  double sy = oy + px * sin(oth) + py * cos(oth);
  double sth = oth + pth;

  px = sx;
  py = sy;
  pth = sth;
}


///////////////////////////////////////////////////////////////////////////
// Convert global to local coords
//
void CEntity::GlobalToLocal(double &px, double &py, double &pth)
{
  // Get the pose of our origin wrt global cs
  double ox, oy, oth;
  GetGlobalPose(ox, oy, oth);

  // Compute pose based on the parent's pose
  double sx =  (px - ox) * cos(oth) + (py - oy) * sin(oth);
  double sy = -(px - ox) * sin(oth) + (py - oy) * cos(oth);
  double sth = pth - oth;

  px = sx;
  py = sy;
  pth = sth;
}


///////////////////////////////////////////////////////////////////////////
// Set the entitys pose in the parent cs
void CEntity::SetPose(double px, double py, double pth)
{
  // if the new position is different, call Property to make the change.
  // the -1 indicates that this change is dirty on all connections
 
  // only change the pose if these are different to the current pose
  if( this->local_px != px || this->local_py != py || this->local_pth != pth )
    {
      stage_pose_t pose;
      pose.x = px;
      pose.y = py;
      pose.a = pth;
      
      Property( -1, STG_PROP_ENTITY_POSE, (char*)&pose, sizeof(pose), NULL ); 
    }
}


///////////////////////////////////////////////////////////////////////////
// Get the entitys pose in the parent cs
void CEntity::GetPose(double &px, double &py, double &pth)
{
  px = this->local_px;
  py = this->local_py;
  pth = this->local_pth;
}


///////////////////////////////////////////////////////////////////////////
// Set the entitys pose in the global cs
void CEntity::SetGlobalPose(double px, double py, double pth)
{
  // Get the pose of our parent in the global cs
  double ox = 0;
  double oy = 0;
  double oth = 0;
  
  if (m_parent_entity) m_parent_entity->GetGlobalPose(ox, oy, oth);
  
  // Compute our pose in the local cs
  double new_x  =  (px - ox) * cos(oth) + (py - oy) * sin(oth);
  double new_y  = -(px - ox) * sin(oth) + (py - oy) * cos(oth);
  double new_th = pth - oth;
  
  SetPose( new_x, new_y, new_th );
}


///////////////////////////////////////////////////////////////////////////
// Get the entitys pose in the global cs
void CEntity::GetGlobalPose(double &px, double &py, double &pth)
{
  // Get the pose of our parent in the global cs
  double ox = 0;
  double oy = 0;
  double oth = 0;
  if (m_parent_entity)
    m_parent_entity->GetGlobalPose(ox, oy, oth);
    
  // Compute our pose in the global cs
  px = ox + this->local_px * cos(oth) - this->local_py * sin(oth);
  py = oy + this->local_px * sin(oth) + this->local_py * cos(oth);
  pth = oth + this->local_pth;
}


////////////////////////////////////////////////////////////////////////////
// Set the entitys velocity in the global cs
void CEntity::SetGlobalVel(double vx, double vy, double vth)
{
  this->vx = vx;
  this->vy = vy;
  this->vth = vth;
}


////////////////////////////////////////////////////////////////////////////
// Get the entitys velocity in the global cs
void CEntity::GetGlobalVel(double &vx, double &vy, double &vth)
{
  vx = this->vx;
  vy = this->vy;
  vth = this->vth;
}


////////////////////////////////////////////////////////////////////////////
// See if the given entity is one of our descendents
bool CEntity::IsDescendent(CEntity *entity)
{
  while (entity)
  {
    entity = entity->m_parent_entity;
    if (entity == this)
      return true;
  }
  return false;
}

void CEntity::SetDirty( int con, char v )
{
  for( int i=0; i< STG_PROPERTY_COUNT; i++ )
    SetDirty( con, (stage_prop_id_t)i, v );
}

void CEntity::SetDirty( stage_prop_id_t prop, char v )
{
  for( int i=0; i< STG_MAX_CONNECTIONS; i++ )
    SetDirty( i, prop, v );
};

void CEntity::SetDirty( int con, stage_prop_id_t prop, char v )
{
  subscriptions[con][prop].dirty = v;
};

// make EVERYTHING dirty
void CEntity::SetDirty( char v )
{
  for( int i=0; i< STG_MAX_CONNECTIONS; i++ )
    SetDirty( i, v ); //  set all props on this channel to dirty = v
};

///////////////////////////////////////////////////////////////////////////
// change the parent 
void CEntity::SetParent(CEntity* new_parent) 
{ 
  m_parent_entity = new_parent; 
};


// scale an array of rectangles so they fit in a unit square
void CEntity::NormalizeRects(  stage_rotrect_t* rects, int num )
{
  // assuming the rectanlges fit in a square +- one billion units
  double minx, miny, maxx, maxy;
  minx = miny = BILLION;
  maxx = maxy = -BILLION;
  
  for( int r=0; r<num; r++ )
    {
      // test the origin of the rect
      if( rects[r].x  < minx ) minx = rects[r].x;
      
      if( rects[r].y < miny ) miny = rects[r].y;
      
      if( rects[r].x  > maxx ) maxx = rects[r].x;
      
      if( rects[r].y > maxy ) maxy = rects[r].y;

      // test the extremes of the rect
          if( (rects[r].x+rects[r].w)  < minx ) 
	minx = (rects[r].x+rects[r].w);
      
      if( (rects[r].y+rects[r].h)  < miny ) 
	miny = (rects[r].y+rects[r].h);
      
      if( (rects[r].x+rects[r].w)  > maxx ) 
	maxx = (rects[r].x+rects[r].w);
      
      if( (rects[r].y+rects[r].h)  > maxy ) 
	maxy = (rects[r].y+rects[r].h);
      
    }
  
  //printf( "xmin %.2f ymin %.2f xmax %.2f ymax %.2f\n", minx, miny, maxx, maxy );
  
  // now normalize all lengths so that the rects all fit inside
  // rectangle from 0,0 to 1,1
  double scalex = maxx - minx;
  double scaley = maxy - miny;
  for( int r=0; r<num; r++ )
    { 
      rects[r].x = (rects[r].x - minx) / scalex;
      rects[r].y = (rects[r].y - miny) / scaley;
      rects[r].w = rects[r].w / scalex;
      rects[r].h = rects[r].h / scaley;
    }
}

void CEntity::Subscribe( int con, stage_subscription_t* subs, int sub_count )  
{
  for( int p=0; p<sub_count; p++ )
    {
      stage_prop_id_t prop_code = subs[p].property;
      stage_subscription_flag_t flag = subs[p].flag;

      if( prop_code == -1 )
	PRINT_WARN( "subscribe to all properties not implemented" );
      else
	{
	  PRINT_DEBUG4( "subscribe %d  to ent %s property %s on connection %d",
			flag, name, 
			//SIOPropString(prop_code), 
			"<dummy>", con );

	  // register the subscription on this channel, for this property
	  subscriptions[con][prop_code].subscribed = flag;
	  subscriptions[con][prop_code].dirty = 
	    (flag == STG_SUBSCRIBED) ? 1 : 0;
	}
    }
}

// clear the subscription data for this channel on me and my children
void CEntity::DestroyConnection( int con )
{
  memset( subscriptions[con], 0, 
	  STG_PROPERTY_COUNT*sizeof(stage_subdirty_t) );

  CHILDLOOP(ch)
    ch->DestroyConnection( con );
}

void CEntity::SetRects( stage_rotrect_t* rects, int num )
{
  // delete any old rects
  if( this->rects )
    {
      RenderRects( false );
      delete[] this->rects;
    }  
  // we just got this many rectangles
  this->rect_count = num;
  
  if( num > 0 )
    { 
      assert( rects );
      // make space for the new rects
      this->rects = new stage_rotrect_t[ num ];
      
      // copy the rects into our local storage
      memcpy( this->rects, rects, num * sizeof(stage_rotrect_t) );
      
      // scale the rects so they fit in a unit square
      this->NormalizeRects( this->rects, this->rect_count );      
      
      // if this is root, add a unit rectangle outline
      if( 0 )//this->m_parent_entity == NULL )
	{
	  // make space for all the rects plus 1
	  stage_rotrect_t* extraone = new stage_rotrect_t[num+1];
	  
	  // set the first rect as a unit square
	  extraone[0].x = 0.0;
	  extraone[0].y = 0.0;
	  extraone[0].a = 0.0;
	  extraone[0].w = 1.0;
	  extraone[0].h = 1.0;
	  
	  // copy the other rects after the unit square
	  memcpy( &extraone[1], this->rects, num * sizeof(stage_rotrect_t) );
	  
	  // free the old rects
	  delete[] this->rects;
	  
	  // point to the new rects
	  this->rects = extraone;
	  this->rect_count = num+1; // remember we have 1 more now
	}
	  

      // draw the rects into the matrix
      this->RenderRects( true );
      
      PRINT_DEBUG2( "created %d rects for entity %s", 
		    this->rect_count, this->name );
    }
  else
    { // no rects 
      this->rects = NULL;
    }
}



// write the entity tree onto the console
void CEntity::Print( int fd, char* prefix )
{
  // TODO - write onto the fd, rather than stdout
  double ox, oy, oth;
  this->GetGlobalPose( ox, oy, oth );

  printf( "model %s type: %s global: [%.2f,%.2f,%.2f]"
	  " local: [%.2f,%.2f,%.2f] vision_return %d )", 
	  prefix,
	  this->name,
	  this->type,
	  ox, oy, oth,
	  local_px, local_py, local_pth,
	  this->vision_return );
	  
  if( this->m_parent_entity == NULL )
    puts( " - ROOT" );
  else
    puts( "" );

  // add an indent to the prefix 
  char* buf = new char[ strlen(prefix) + 1 ];
  sprintf( buf, "\t%s", prefix );

  CHILDLOOP( ch )
    ch->Print( fd, buf );
}

// return true if this property is subscribed on any connection
bool CEntity::IsSubscribed( stage_prop_id_t prop )
{
  for( int con=0; con < STG_MAX_CONNECTIONS; con++ )
    if( subscriptions[con][prop].subscribed )
      return true;
  
  return false;
}


void CEntity::GetStatusString( char* buf, int buflen )
{
  double x, y, th;
  this->GetGlobalPose( x, y, th );
  
  // check for overflow
  assert( -1 !=  snprintf( buf, buflen, 
			   "Pose(%.2f,%.2f,%.2f) Stage(%s:%s)",
			   x, y, th, this->name, this->type ) );
}  


void CEntity::RenderRects( bool render )
{
  if( !CEntity::matrix )
    return;
  
  stage_rotrect_t glob;
  
  int r;
  for( r=0; r<this->rect_count; r++ )
    {
      GetGlobalRect( &glob, &(this->rects[r]) );
      CEntity::matrix->SetRectangle( glob.x, glob.y, glob.a, 
				     glob.w, glob.h, this, render );
    }  
}


int CEntity::GetData( char* data, size_t* len )
{
  //PRINT_DEBUG4( "pasting ent %s's data (%d bytes) from %p into %p",
  //	this->name, buffer_data.len, buffer_data.data, data ); 
  
  memcpy( data, buffer_data.data, buffer_data.len );
  *len = buffer_data.len;
  return 0;
}

int CEntity::GetCommand( char* data, size_t* len )
{
  memcpy( data, buffer_cmd.data, buffer_cmd.len );
  *len = buffer_cmd.len;
  return 0;
}


#ifdef INCLUDE_RTK2
#include "rtkgui.hh"

///////////////////////////////////////////////////////////////////////////
// Initialise the rtk gui
int CEntity::RtkStartup( rtk_canvas_t* canvas )
{
  PRINT_DEBUG2( "RTKSTARTUP ent %s:%s", this->name, this->type );
  
  // store the GUI items for later
  this->canvas = canvas;

  if( canvas == NULL )
    {
      PRINT_WARN1( "rtk startup for ent %s no canvas!", this->name );
      return 0; // it's ok though..
    }

  if( this->fig )
    {
      PRINT_WARN2( " fig already exists for  ent %s:%s. deleting it.", 
		   this->name, this->type );
      
      rtk_fig_destroy( this->fig );
      fig_count--;
    }

  // Create a figure representing this entity.
  // if we have a parent, attach the new figure to the parent's figure
  this->fig = rtk_fig_create( canvas, 
			      m_parent_entity ? m_parent_entity->fig : NULL,
			      50);
  
  assert( this->fig );

  fig_count++;
  
  PRINT_DEBUG3( "FIG COUNT %d for  ent %s:%s", 
	       fig_count, this->name, this->type );
  
  // Set the mouse handler
  this->fig->userdata = this;
  rtk_fig_add_mouse_handler(this->fig, RtkOnMouse);

  // add this device to the world's device menu 
  //this->m_world->AddToDeviceMenu( this, true); 
    
  // visible by default
  rtk_fig_show( this->fig, true );

  // Set the color
  rtk_fig_color_rgb32(this->fig, this->color);

  if( m_parent_entity == NULL )
    // the root device is drawn from the corner, not the center
    rtk_fig_origin( this->fig, local_px, local_py, local_pth );
  else
    // put the figure's origin at the entity's position
    rtk_fig_origin( this->fig, local_px, local_py, local_pth );



#ifdef RENDER_INITIAL_BOUNDING_BOXES
  double xmin, ymin, xmax, ymax;
  xmin = ymin = 999999.9;
  xmax = ymax = 0.0;
  this->GetBoundingBox( xmin, ymin, xmax, ymax );
  
  rtk_fig_t* boundaries = rtk_fig_create( canvas, NULL, 99);
  double width = xmax - xmin;
  double height = ymax - ymin;
  double xcenter = xmin + width/2.0;
  double ycenter = ymin + height/2.0;

  rtk_fig_rectangle( boundaries, xcenter, ycenter, 0, width, height, 0 ); 

   
#endif
   
  // everything except the root object has a label
  //if( m_parent_entity )
  //{
    // Create the label
    // By default, the label is not shown
    this->fig_label = rtk_fig_create( canvas, NULL, 51);
    rtk_fig_show(this->fig_label, 0); 
    rtk_fig_movemask(this->fig_label, 0);
      
    char label[1024];
    char tmp[1024];
      
    label[0] = 0;

    snprintf(tmp, sizeof(tmp), "%s", this->name );
    
    strncat(label, tmp, sizeof(label));
      
    rtk_fig_color_rgb32(this->fig, this->color);
    //rtk_fig_origin( this->fig_label, 0.05, 1.0, 0 );
    rtk_fig_text(this->fig_label, 0,0,0, label);
      
    // attach the label to the main figure
    // rtk will draw the label when the mouse goes over the figure
    // TODO: FIX
    //this->fig->mouseover_fig = fig_label;
    
    // we can be moved only if we are on the root node
    if (m_parent_entity != (CEntity*)CEntity::root )
      rtk_fig_movemask(this->fig, 0);
    else
      rtk_fig_movemask(this->fig, this->movemask);  

    //}

 
  if( this->grid_enable )
    {
      this->fig_grid = rtk_fig_create(canvas, this->fig, -49);
      if ( this->grid_minor > 0)
	{
	  rtk_fig_color( this->fig_grid, 0.9, 0.9, 0.9);
	  rtk_fig_grid( this->fig_grid, this->origin_x, this->origin_y, 
			this->size_x, this->size_y, grid_minor);
	}
      if ( this->grid_major > 0)
	{
	  rtk_fig_color( this->fig_grid, 0.75, 0.75, 0.75);
	  rtk_fig_grid( this->fig_grid, this->origin_x, this->origin_y, 
			this->size_x, this->size_y, grid_major);
	}
      rtk_fig_show( this->fig_grid, 1);
    }
  else
    this->fig_grid = NULL;
  

  PRINT_DEBUG1( "rendering %d rectangles", this->rect_count );
  
  // now create GUI rects
  int r;
  for( r=0; r<this->rect_count; r++ )
    {
      stage_rotrect_t* src = &this->rects[r];
      double x,y,a,w,h;
      
      x = ((src->x + src->w/2.0) * this->size_x) - this->size_x/2.0 + origin_x;
      y = ((src->y + src->h/2.0) * this->size_y) - this->size_y/2.0 + origin_y;
      a = src->a;
      w = src->w * this->size_x;
      h = src->h * this->size_y;
      
      //if( this->m_parent_entity == NULL )
      //rtk_fig_rectangle(this->fig, x,y,a,w,h, 1 ); 
      //else
	rtk_fig_rectangle(this->fig, x,y,a,w,h, 0 ); 
    }

  // add rects showing transducer positions
  for( int s=0; s< this->transducer_count; s++ )
    rtk_fig_rectangle( this->fig, 
		       this->transducers[s][0], 
		       this->transducers[s][1], 
		       this->transducers[s][2], this->size_x/10.0, this->size_y/10.0, 0 ); 

  
  // we're done - now do our childre
  CHILDLOOP( child ) 
    if( child->RtkStartup( canvas ) == -1 )
      {
	PRINT_ERR( "failed to rtkstartup child" );
	return -1;
      }


  PRINT_DEBUG1( "RTK STARTUP DONE (fig = %p)", this->fig );

  return 0;
}



///////////////////////////////////////////////////////////////////////////
// Finalise the rtk gui
void CEntity::RtkShutdown()
{
  PRINT_DEBUG2( "RTKSHUTDOWN ent %s:%s", this->name, this->type );

  // do our children first
  CHILDLOOP( child )
    child->RtkShutdown();
  
  // Clean up the figure we created
  if( this->fig ) 
    {
      rtk_fig_destroy(this->fig);
      this->fig = NULL;
     
      fig_count--;
      
      PRINT_DEBUG3( "FIG COUNT %d for  ent %s:%s)", 
		   fig_count, this->name, this->type );
    }


  if( this->fig_label ) rtk_fig_destroy(this->fig_label);
  if( this->fig_grid ) rtk_fig_destroy( this->fig_grid );

  this->fig = NULL;
  this->fig_label = NULL;
  this->fig_grid = NULL;
} 


///////////////////////////////////////////////////////////////////////////
// Update the rtk gui
int CEntity::RtkUpdate()
{
  PRINT_DEBUG2( "RTK update for ent %s (fig = %p)", 
		this->name, this->fig  );
  

  //assert( this->fig );
  // if we have no figure, do nothing
  if( !this->fig )
    {
      PRINT_WARN1( "no fig for ent %s", this->name );
      return 0;
    }

  //CHILDLOOP( child ) child->RtkUpdate();

  // TODO this is nasty and inefficient - figure out a better way to
  // do this  

  // if we're not looking at this device, hide it 
  //if( !m_world->ShowDeviceBody( this->lib_entry->type_num ) )
  //{
  //rtk_fig_show(this->fig, false);
  //}
  //else // we need to show and update this figure
  //{
  //rtk_fig_show( this->fig, true ); 

  //PRINT_DEBUG1( "RTK update for ent %s done", this->name );  

  return 0; // success
}

#endif
