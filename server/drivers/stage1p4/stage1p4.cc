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

// STAGE-1.4 DRIVER BASE CLASS  /////////////////////////////////
// creates a single static Stage client

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
#include <sys/soundcard.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h> 
#include <unistd.h>
#include <float.h> // for DBL_MAX
#include <pthread.h>

extern int global_argc;
extern char** global_argv;

#include "image.hh" // TODO - replace Neil's old image code 
//extern "C" {
//#include <pam.h> // TODO - use a portable image-reading library
//}

#include "stage1p4.h"

// init static vars
char* Stage1p4::world_file = DEFAULT_STG_WORLDFILE;
stg_client_t* Stage1p4::stage_client = NULL;
stg_name_id_t* Stage1p4::created_models = NULL;
int Stage1p4::created_models_count = 0;


// constructor
//
Stage1p4::Stage1p4(char* interface, ConfigFile* cf, int section, 
		   size_t datasz, size_t cmdsz, int rqlen, int rplen) :
  CDevice(datasz,cmdsz,rqlen,rplen)
{
  PLAYER_TRACE1( "Stage1p4 device created for interface %s\n", interface );
  
  this->section = section;
  
  // load my name from the config file
  const char* name = cf->ReadString( section, "name", "<no name>" );
  
  PLAYER_MSG1( "stage1p4 creating device name \"%s\"", name );
  
  if(  Stage1p4::stage_client == NULL )
    {
      Stage1p4::world_file = 
	(char*)cf->ReadString(section, "worldfile", DEFAULT_STG_WORLDFILE);
      
      int stage_port = 
	cf->ReadInt(section, "port", STG_DEFAULT_SERVER_PORT);
      
      char* stage_host = 
	(char*)cf->ReadString(section, "host", DEFAULT_STG_HOST);
            
      Stage1p4::stage_client = 
	this->CreateStageClient(stage_host, stage_port, world_file);	
    }
  
  // now the Stage worldfile has been read and all the devices created.
  // I look up my name to get a Stage model id.

  this->stage_id = -1; // invalid
  for( int i=0; i < created_models_count; i++ )
    if( strcmp( created_models[i].name, name ) == 0 )
      {
	this->stage_id =  created_models[i].stage_id;
	break;
      }
  
    if( stage_id == -1 )
      PLAYER_ERROR1( "stage1p4: device name \"%s\" doesn't match a Stage model", name );
    else
      PLAYER_MSG2( "stage1p4: device name \"%s\" matches stage model %d",
		   name, this->stage_id );    
}

// destructor
Stage1p4::~Stage1p4()
{
  if( this->stage_client )
    this->DestroyStageClient( this->stage_client );
}

stg_client_t* Stage1p4::CreateStageClient( char* host, int port, char* world )
{
  // TODO - initialize the bitmap library
  //pnm_init( &global_argc, global_argv );

  PLAYER_MSG2( "Creating client to Stage server on %s:%d", host, port );

  stg_client_t* cli = stg_client_create( host, port );
  assert( cli );

  PLAYER_MSG1( "Uploading world from \"%s\"", world );

  // TODO - move all this into the worldfile library
  // load a worldfile
  CWorldFile wf;
  wf.Load( world );
  
  stg_world_create_t world_cfg;
  strncpy(world_cfg.name, wf.ReadString( 0, "name", world ), STG_TOKEN_MAX );
  strncpy(world_cfg.token, this->world_file , STG_TOKEN_MAX );
  world_cfg.width =  wf.ReadTupleFloat( 0, "size", 0, 10.0 );
  world_cfg.height =  wf.ReadTupleFloat( 0, "size", 1, 10.0 );
  world_cfg.resolution = wf.ReadFloat( 0, "resolution", 0.1 );
  stg_id_t root = stg_world_create( cli, &world_cfg );
  
  // for every worldfile section, we may need to store a model ID in
  // order to resolve parents
  created_models_count = wf.GetEntityCount();
  created_models = new stg_name_id_t[ created_models_count ];
  
  // the default parent of every model is root
  for( int m=0; m<created_models_count; m++ )
    {
      created_models[m].stage_id = root;
      strncpy( created_models[m].name, "root", STG_TOKEN_MAX );
    }

  // Iterate through sections and create entities as required
  for (int section = 1; section < wf.GetEntityCount(); section++)
  {
    if( strcmp( "gui", wf.GetEntityType(section) ) == 0 )
      {
	PLAYER_WARN( "gui section not implemented" );
      }
    else
      {
	// TODO - handle the line numbers
	const int line = wf.ReadInt(section, "line", -1);

	stg_id_t parent = created_models[wf.GetEntityParent(section)].stage_id;

	PRINT_MSG1( "creating child of parent %d", parent );
	
	stg_entity_create_t child;
	strncpy(child.name, wf.ReadString(section,"name", "" ), 
		STG_TOKEN_MAX);
	strncpy(child.token, wf.GetEntityType(section), 
		STG_TOKEN_MAX );
	strncpy(child.color, wf.ReadString(section,"color",""), 
		STG_TOKEN_MAX);
	child.parent_id = parent; // make a new entity on the root 
	
	// decode the token into a type number
	//child.type = stg_model_type_from_string( child.token );
		
	// warn the user if no name was specified
	if( strcmp( child.name, "" ) == 0 )
	  PLAYER_MSG2( "stage1p4: model %s (line %d) has no name specified. Player will not be able to access this device", child.token, line );
	
	stg_id_t banana = stg_model_create( cli, &child );
	
	// associate the name 
	strncpy( created_models[section].name, child.name, STG_TOKEN_MAX );
	
	PLAYER_MSG3( "stage1p4: associating section %d name %s "
		     "with stage model %d",
		     section, 
		     created_models[section].name, 
		     created_models[section].stage_id );
	
	// remember the model id for this section
	created_models[section].stage_id = banana;
	
	PLAYER_MSG1( "created model %d", banana );

	// TODO - is there a nicer way to handle unspecified settings?
	// right now i set stupid defaults and test for them

	stg_size_t sz;
	sz.x = wf.ReadTupleFloat( section, "size", 0, -99.0 );
	sz.y = wf.ReadTupleFloat( section, "size", 1, -99.0 );

	if( sz.x != -99 && sz.y != 99 )
	  stg_model_set_size( cli, banana, &sz );
	
	stg_velocity_t vel;
	vel.x = wf.ReadTupleFloat( section, "velocity", 0, 0.0 );
	vel.y = wf.ReadTupleFloat( section, "velocity", 1, 0.0 );
	vel.a = wf.ReadTupleFloat( section, "velocity", 2, 0.0 );
	stg_model_set_velocity( cli, banana, &vel );
	
	stg_pose_t pose;
	pose.x = wf.ReadTupleFloat( section, "pose", 0, 0.0 );
	pose.y = wf.ReadTupleFloat( section, "pose", 1, 0.0 );
	pose.a = wf.ReadTupleFloat( section, "pose", 2, 0.0 );
	stg_model_set_pose( cli, banana, &pose );

	const char* bitmapfile = wf.ReadString(section,"bitmap", "" );

	stg_rotrect_array_t* rects = CreateRectsFromBitMapFile( bitmapfile );
	if( rects ) stg_model_set_rects( cli, banana, rects );
	
	// Load the transducers
	int transducer_count = wf.ReadInt(section, "transducer_count", 0);
	if (transducer_count > 0)
	  {
	    stg_transducer_t* trans = new stg_transducer_t[transducer_count];
	    memset( trans, 0, sizeof(stg_transducer_t) * transducer_count );
	    
	    int i;
	    for (i = 0; i < transducer_count; i++)
	      {
		char key[64];
		snprintf(key, sizeof(key), "transducer[%d]", i);
		trans[i].pose.x = wf.ReadTupleLength(section,key,0,0);
		trans[i].pose.y = wf.ReadTupleLength(section,key,1,0);
		trans[i].pose.a = wf.ReadTupleAngle(section, key,2,0);
		trans[i].size.x = wf.ReadTupleLength(section,key,3,0);
		trans[i].size.y = wf.ReadTupleLength(section,key,4,0);
	      }
	    
	    stg_model_set_transducers( cli, banana, trans, transducer_count );
	  }
	
	// check to see if this model wants to show up in the neighbor sensor
	int neighbor_return = wf.ReadInt( section, "neighbor", false );
	stg_model_set_neighbor_return( cli, banana, &neighbor_return );
	
	const char* laserstring = wf.ReadString( section, "laser_return", "" );
	
	stg_laser_return_t lret;
	if( strcmp( laserstring, "visible" ) == 0 )
	  {
	    lret = LaserVisible;
	    stg_model_set_laser_return( cli, banana, &lret );
	  }
	else if( strcmp( laserstring, "invisible" ) == 0 )
	  {
	    lret = LaserTransparent;
	    stg_model_set_laser_return( cli, banana, &lret );
	  }
	else if( strcmp( laserstring, "bright" ) == 0 )
	  {
	    lret = LaserBright;
	    stg_model_set_laser_return( cli, banana, &lret );
	  }

	// TODO - all the other sensor return values here

	stg_bounds_t bounds;	
	const char* key = "neighbor_range_bounds";
	bounds.min = wf.ReadTupleLength(section,key,0,-1);
	bounds.max = wf.ReadTupleLength(section,key,1,-1);
	
	// if we found a value, poke it into Stage
	if( bounds.min != -1 )
	  stg_model_set_neighbor_bounds(cli,banana,&bounds);
      } 
  }
  return cli;
}

// compresses the bitmap data into an array of rectangles, allocating
// storag as it goes. caller must free the rects (use stg_rect_array_free()) 
// this code belongs in a Stage library - needs rewriting in C,
// without the dependency on Neil's Nimage code 
stg_rotrect_array_t* 
Stage1p4::CreateRectsFromBitMapFile( const char* bitmapfile )
{
  if( strcmp( bitmapfile, "" ) == 0 )
    return NULL;
  
  Nimage* img = new Nimage();
  
  PLAYER_MSG1("Loading bitmap file \"%s\"", bitmapfile );
  
  int len = strlen(bitmapfile);
  if( strcmp( &(bitmapfile[ len - 7 ]), ".pnm.gz" ) == 0 )
    {
      if (!img->load_pnm_gz(bitmapfile))
	return NULL;
    }
  else 
    {
      if (!img->load_pnm(bitmapfile))
	return NULL;
    }
  
/*
    FILE *bitmap = fopen(bitmapfile, "r" );
    if( bitmap == NULL )
    {
    PLAYER_WARN1("failed to open bitmap file \"%s\"", bitmapfile);
    perror( "fopen error" );
    }
    
    struct pam inpam;
    pnm_readpaminit(bitmap, &inpam, sizeof(inpam)); 
    
    printf( "read image %dx%dx%d",
    inpam.width, 
    inpam.height, 
    inpam.depth );
  */
  
  // convert the bitmap to rects and poke them into the model
  double sx = 1.0;
  double sy = 1.0;
  
  double size_x = 1.0;
  double size_y = 1.0;

  double crop_ax = -DBL_MAX;
  double crop_ay = -DBL_MAX;
  double crop_bx = +DBL_MAX;
  double crop_by = +DBL_MAX;
 
  // RTV - this box-drawing algorithm compresses hospital.world from
  // 104,000+ pixels to 5,757 rectangles. it's not perfect but pretty
  // darn good with bitmaps featuring lots of horizontal and vertical
  // lines - such as most worlds. Also combined matrix & gui
  // rendering loops.  hospital.pnm.gz now loads more than twice as
  // fast and redraws waaaaaay faster. yay!
  
  stg_rotrect_array_t* rect_array = stg_rotrect_array_create();
  
  for (int y = 0; y < img->height; y++)
    {
      for (int x = 0; x < img->width; x++)
	{
	  //m_world->Ticker();
	  
	  if (img->get_pixel(x, y) == 0)
	    continue;
	  
	  // a rectangle starts from this point
	  int startx = x;
	  int starty = img->height - y;
	  int height = img->height; // assume full height for starters
	  
	  // grow the width - scan along the line until we hit an empty pixel
	  for( ;  img->get_pixel( x, y ) > 0; x++ )
	    {
	      // handle horizontal cropping
	      double ppx = x * sx; 
	      if (ppx < crop_ax || ppx > crop_bx)
		continue;
	      
	      // look down to see how large a rectangle below we can make
	      int yy  = y;
	      while( (img->get_pixel( x, yy ) > 0 ) 
		     && (yy < img->height) )
		{ 
		  // handle vertical cropping
		  double ppy = (img->height - yy) * sy;
		  if (ppy < crop_ay || ppy > crop_by)
		    continue;
		  
		  yy++; 
		} 	      
	      // now yy is the depth of a line of non-zero pixels
	      // downward we store the smallest depth - that'll be the
	      // height of the rectangle
	      if( yy-y < height ) height = yy-y; // shrink the height to fit
	    } 
	  
	  int width = x - startx;
	  
	  // delete the pixels we have used in this rect
	  img->fast_fill_rect( startx, y, width, height, 0 );
	  
	  stg_rotrect_t rect;
	  rect.x = startx;
	  rect.y = starty;
	  rect.a = 0.0;
	  rect.w = width;
	  rect.h = -height;
	  stg_rotrect_array_append( rect_array, &rect ); // add it to the array  
	}
    }
  delete img;
  
  return rect_array;
}


int Stage1p4::DestroyStageClient( stg_client_t* cli )
{ 
  PLAYER_MSG0( "STAGE DRIVER DESTROY CLIENT" );
  stg_client_free( cli );
  return 0;
}


int 
Stage1p4::Setup()
{
  PLAYER_MSG0( "STAGE DRIVER SETUP" );
  return 0;
}

int 
Stage1p4::Shutdown()
{
  PLAYER_MSG0( "STAGE DRIVER SHUTDOWN" );
  return 0;
}

// END BASE CLASS

