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
#include "playercommon.h"
#include "drivertable.h"
#include "player.h"
#include "stage1p4.h"

#include "stg_time.h"
extern PlayerTime* GlobalTime;

extern int global_argc;
extern char** global_argv;


// DRIVER FOR SIMULATION INTERFACE //////////////////////////////////////////////////////

class StgSimulation:public Stage1p4
{
public:
  StgSimulation(char* interface, ConfigFile* cf, int section);

};

StgSimulation::StgSimulation(char* interface, ConfigFile* cf, int section ) 
  : Stage1p4( interface, cf, section, 
	      sizeof(player_simulation_data_t), 
	      sizeof(player_simulation_cmd_t), 1, 1 )
{
  PLAYER_MSG1( "constructing StgSimulation with interface %s", interface );
  
  // just do this startup stuff in the simulation once - it is hared
  // by all instances. do this in the constructor and not in Setup()
  // so the Stage window gets shown and populated without having to
  // connect a client.
  
  if( Stage1p4::stage_client == NULL )
    {
      const int stage_port = 
	config->ReadInt(section, "port", STG_DEFAULT_SERVER_PORT);
      
      const char* stage_host = 
	config->ReadString(section, "host", STG_DEFAULT_SERVER_HOST);
      
      printf( "Stage1p4: Creating client to Stage server on %s:%d\n", 
	      stage_host, stage_port );
      
      Stage1p4::stage_client = stg_client_create();
      assert(Stage1p4::stage_client);

      stg_client_connect( Stage1p4::stage_client, (char*)stage_host, stage_port );
      
      // load a worldfile
      Stage1p4::worldfile_name = 
	(char*)config->ReadString(section, "worldfile", STG_DEFAULT_WORLDFILE);
      
      PLAYER_TRACE1( "Loading worldfile \"%s\"", worldfile_name );      
      Stage1p4::wf.Load( Stage1p4::worldfile_name );
      PLAYER_TRACE0( "done." );


      // create a passel of Stage models based on the worldfile
      //

      int section = 0;
      
      Stage1p4::world_name = wf.ReadString(section, "name", (char*)"Player world" );
      
      double resolution = 
	wf.ReadFloat(0, "resolution", 0.02 ); // 2cm default
      resolution = 1.0 / resolution; // invert res to ppm
      
      double interval_real = wf.ReadFloat(section, "interval_real", 0.1 );
      double interval_sim = wf.ReadFloat(section, "interval_sim", 0.1 );
      
      // create a single world
      stg_world_t* world = 
	stg_client_createworld( Stage1p4::stage_client, 
				0,
				stg_token_create( world_name, STG_T_NUM, 99 ),
				resolution, interval_sim, interval_real );
      
      // create a special model for the background
      stg_token_t* token = stg_token_create( "root", STG_T_NUM, 99 );
      stg_model_t* root = stg_world_createmodel( world, NULL, 0, token );
      
      stg_size_t sz;
      sz.x = wf.ReadTupleLength(section, "size", 0, 10.0 );
      sz.y = wf.ReadTupleLength(section, "size", 1, 10.0 );
      stg_model_prop_with_data( root, STG_PROP_SIZE, &sz, sizeof(sz) );
      
      stg_pose_t pose;
      pose.x = sz.x/2.0;
      pose.y = sz.y/2.0;
      pose.a = 0.0;
      stg_model_prop_with_data( root, STG_PROP_POSE, &pose, sizeof(pose) );
      
      stg_movemask_t mm = 0;
      stg_model_prop_with_data( root, STG_PROP_MOVEMASK, &mm, sizeof(mm) ); 
      
      const char* colorstr = wf.ReadString(section, "color", "black" );
      stg_color_t color = stg_lookup_color( colorstr );
      stg_model_prop_with_data( root, STG_PROP_COLOR, &color,sizeof(color));
      
      const char* bitmapfile = wf.ReadString(section, "bitmap", NULL );
      if( bitmapfile )
	{
	  stg_rotrect_t* rects = NULL;
	  int num_rects = 0;
	  stg_load_image( bitmapfile, &rects, &num_rects );
	  
	  // convert rects to an array of lines
	  int num_lines = 4 * num_rects;
	  stg_line_t* lines = stg_rects_to_lines( rects, num_rects );
	  stg_normalize_lines( lines, num_lines );
	  stg_scale_lines( lines, num_lines, sz.x, sz.y );
	  stg_translate_lines( lines, num_lines, -sz.x/2.0, -sz.y/2.0 );
	  
	  stg_model_prop_with_data( root, STG_PROP_LINES, 
				    lines, num_lines * sizeof(stg_line_t ));
	  
	  free( lines );
	}
      
      stg_bool_t boundary;
      boundary = wf.ReadInt(section, "boundary", 1 );
      stg_model_prop_with_data(root, STG_PROP_BOUNDARY, &boundary,sizeof(boundary));
      
      stg_bool_t grid;
      grid = wf.ReadInt(section, "grid", 1 );
      stg_model_prop_with_data(root, STG_PROP_GRID, &grid, sizeof(grid) );

    // Iterate through sections and create client-side models
  for (int section = 1; section < wf.GetEntityCount(); section++)
    {
      // a model has the macro name that defined it as it's name
      // unlesss a name is explicitly defined.  TODO - count instances
      // of macro names so we can autogenerate unique names
      const char *default_namestr = wf.GetEntityType(section);      
      const char *namestr = wf.ReadString(section, "name", default_namestr);
      stg_token_t* token = stg_token_create( namestr, STG_T_NUM, 99 );
      stg_model_t* mod = stg_world_createmodel( world, NULL, section, token );
      //stg_world_addmodel( world, mod );

      stg_pose_t pose;
      pose.x = wf.ReadTupleLength(section, "pose", 0, 0);
      pose.y = wf.ReadTupleLength(section, "pose", 1, 0);
      pose.a = wf.ReadTupleAngle(section, "pose", 2, 0);      
      stg_model_prop_with_data( mod, STG_PROP_POSE, &pose, sizeof(pose) );
      
      stg_size_t sz;
      sz.x = wf.ReadTupleLength(section, "size", 0, 0.4 );
      sz.y = wf.ReadTupleLength(section, "size", 1, 0.4 );
      stg_model_prop_with_data( mod, STG_PROP_SIZE, &sz, sizeof(sz) );
      
      stg_bool_t nose;
      nose = wf.ReadInt( section, "nose", 0 );
      stg_model_prop_with_data( mod, STG_PROP_NOSE, &nose, sizeof(nose) );

      stg_bool_t grid;
      grid = wf.ReadInt( section, "grid", 0 );
      stg_model_prop_with_data( mod, STG_PROP_GRID, &grid, sizeof(grid) );
      
      stg_bool_t boundary;
      boundary = wf.ReadInt( section, "boundary", 0 );
      stg_model_prop_with_data( mod, STG_PROP_BOUNDARY, &boundary,sizeof(boundary));
      
      stg_laser_config_t lconf;
      lconf.pose.x = wf.ReadTupleLength(section, "laser", 0, 0);
      lconf.pose.y = wf.ReadTupleLength(section, "laser", 1, 0);
      lconf.pose.a = wf.ReadTupleAngle(section, "laser", 2, 0);
      lconf.size.x = wf.ReadTupleLength(section, "laser", 3, 0);
      lconf.size.y = wf.ReadTupleLength(section, "laser", 4, 0);
      lconf.range_min = wf.ReadTupleLength(section, "laser", 5, 0);
      lconf.range_max = wf.ReadTupleLength(section, "laser", 6, 8);
      lconf.fov = wf.ReadTupleAngle(section, "laser", 7, 180);
      lconf.samples = (int)wf.ReadTupleFloat(section, "laser", 8, 180);
      
      stg_model_prop_with_data( mod, STG_PROP_LASERCONFIG, &lconf,sizeof(lconf));
  
      const char* colorstr = wf.ReadString( section, "color", "red" );
      if( colorstr )
	{
	  stg_color_t color = stg_lookup_color( colorstr );
	  PRINT_DEBUG2( "stage color %s = %X", colorstr, color );
	  stg_model_prop_with_data( mod, STG_PROP_COLOR, &color,sizeof(color));
	}

      const char* bitmapfile = wf.ReadString( section, "bitmap", NULL );
      if( bitmapfile )
	{
	  stg_rotrect_t* rects = NULL;
	  int num_rects = 0;
	  stg_load_image( bitmapfile, &rects, &num_rects );
	  
	  // convert rects to an array of lines
	  int num_lines = 4 * num_rects;
	  stg_line_t* lines = stg_rects_to_lines( rects, num_rects );
	  stg_normalize_lines( lines, num_lines );
	  stg_scale_lines( lines, num_lines, sz.x, sz.y );
	  stg_translate_lines( lines, num_lines, -sz.x/2.0, -sz.y/2.0 );
	  
	  stg_model_prop_with_data( mod, STG_PROP_LINES, 
				   lines, num_lines * sizeof(stg_line_t ));
	  	  
	  free( lines );
	  
	}

      // Load the geometry of a ranger array
      int scount = wf.ReadInt( section, "scount", 0);
      if (scount > 0)
	{
	  char key[256];
	  stg_ranger_config_t* configs = (stg_ranger_config_t*)
	    calloc( sizeof(stg_ranger_config_t), scount );

	  int i;
	  for(i = 0; i < scount; i++)
	    {
	      snprintf(key, sizeof(key), "spose[%d]", i);
	      configs[i].pose.x = wf.ReadTupleLength(section, key, 0, 0);
	      configs[i].pose.y = wf.ReadTupleLength(section, key, 1, 0);
	      configs[i].pose.a = wf.ReadTupleAngle(section, key, 2, 0);

	      snprintf(key, sizeof(key), "ssize[%d]", i);
	      configs[i].size.x = wf.ReadTupleLength(section, key, 0, 0.01);
	      configs[i].size.y = wf.ReadTupleLength(section, key, 1, 0.05);

	      snprintf(key, sizeof(key), "sbounds[%d]", i);
	      configs[i].bounds_range.min = 
		wf.ReadTupleLength(section, key, 0, 0);
	      configs[i].bounds_range.max = 
		wf.ReadTupleLength(section, key, 1, 5.0);

	      snprintf(key, sizeof(key), "sfov[%d]", i);
	      configs[i].fov = wf.ReadAngle(section, key, 30 );
	    }
	  
	  PRINT_WARN1( "loaded %d ranger configs", scount );	  
	  stg_model_prop_with_data( mod, STG_PROP_RANGERCONFIG,
				   configs, scount * sizeof(stg_ranger_config_t) );

	  free( configs );
	}
      
      
      int linecount = wf.ReadInt( section, "line_count", 0 );
      if( linecount > 0 )
	{
	  char key[256];
	  stg_line_t* lines = (stg_line_t*)calloc( sizeof(stg_line_t), linecount );
	  int l;
	  for(l=0; l<linecount; l++ )
	    {
	      snprintf(key, sizeof(key), "line[%d]", l);

	      lines[l].x1 = wf.ReadTupleLength(section, key, 0, 0);
	      lines[l].y1 = wf.ReadTupleLength(section, key, 1, 0);
	      lines[l].x2 = wf.ReadTupleLength(section, key, 2, 0);
	      lines[l].y2 = wf.ReadTupleLength(section, key, 3, 0);	      
	    }
	  
	  printf( "NOTE: loaded line %d/%d (%.2f,%.2f - %.2f,%.2f)\n",
		  l, linecount, 
		  lines[l].x1, lines[l].y1, 
		  lines[l].x2, lines[l].y2 ); 
	  
	  stg_model_prop_with_data( mod, STG_PROP_LINES,
				   lines, linecount * sizeof(stg_line_t) );
	  
	  free( lines );
	}

      stg_velocity_t vel;
      vel.x = wf.ReadTupleLength(section, "velocity", 0, 0);
      vel.y = wf.ReadTupleLength(section, "velocity", 1, 0);
      vel.a = wf.ReadTupleAngle(section, "velocity", 2, 0);      
      stg_model_prop_with_data( mod, STG_PROP_VELOCITY, &vel, sizeof(vel) );

      
    }
  
  printf( "building client-side models done.\n" );
  
  puts( "uploading worldfile to server" );
  stg_client_push( stage_client );
  puts( "uploading done" );
  
      //PRINT_WARN1( "TIME SUBSCRIBE world id %d", 
      //   Stage1p4::stage_client->world->id );
      
      /*      
	      PRINT_WARN( "waiting for clock data" );
	      while( Stage1p4::stage_client->world->time == 0.0 )
	      stg_client_read( Stage1p4::stage_client );
	      
	      PRINT_WARN1( "time %.6f - clock ok ",  
	      Stage1p4::stage_client->world->time );
      */
      
      // steal the global clock - a bit aggressive, but a simple approach
  if( GlobalTime ) delete GlobalTime;
  assert( (GlobalTime = new StgTime( Stage1p4::stage_client ) ));

  // subscribe to something so we get the clock updates
  stg_model_subscribe( root, STG_PROP_TIME, 0.1 );
  
  // start the simulation
  //stg_msg_t*  msg = stg_msg_create( STG_MSG_SERVER_RUN, NULL, 0 );
  //stg_fd_msg_write( this->stage_client->pfd.fd, msg );
  //stg_msg_destroy( msg );
  
    } 
  
}

CDevice* StgSimulation_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_SIMULATION_STRING))
    {
      PLAYER_ERROR1("driver \"stg_simulation\" does not support interface \"%s\"\n",
		    interface);
      return(NULL);
    }
  else 
    return((CDevice*)(new StgSimulation(interface, cf, section)));
}


void StgSimulation_Register(DriverTable* table)
{
  table->AddDriver("stg_simulation", PLAYER_ALL_MODE, StgSimulation_Init);
}

//////////////////////////////////////////////////////////////////////////////


