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
 * Desc: The RTK gui implementation
 * Author: Richard Vaughan, Andrew Howard
 * Date: 7 Dec 2000
 * CVS info: $Id$
 */

#if HAVE_CONFIG_H
#  include <config.h>
#endif

// this should go when I get the autoconf set up properly
#ifdef INCLUDE_RTK2

//#undef DEBUG
//#undef VERBOSE
#define DEBUG 
#define VERBOSE

#include <errno.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <netdb.h>

#include <fstream>
#include <iostream>

#include "rtk.h"
#include "root.hh"
#include "matrix.hh"
#include "rtkgui.hh"
extern int quit;

// defaults

#define STG_DEFAULT_WINDOW_WIDTH 400
#define STG_DEFAULT_WINDOW_HEIGHT 400
#define STG_DEFAULT_WINDOW_XORIGIN (STG_DEFAULT_WINDOW_WIDTH/2)
#define STG_DEFAULT_WINDOW_YORIGIN (STG_DEFAULT_WINDOW_HEIGHT/2)
#define STG_DEFAULT_PPM 40
#define STG_DEFAULT_SHOWGRID 1
#define STG_DEFAULT_SHOWSUBSONLY 1

// Timing info for the gui.
// [update_interval] is the time to wait between GUI updates (simulated seconds).
// [root_check_interval] is the time to wait between fitting the root entity nicely into the canvas
double rtkgui_update_interval = 0.01;
double rtkgui_fit_interval = 0.5; 

// Basic GUI elements
rtk_canvas_t *canvas = NULL; // the canvas
rtk_app_t *app = NULL; // the RTK/GTK application

rtk_fig_t* matrix_fig = NULL;

// The file menu
rtk_menu_t *file_menu= NULL;;
rtk_menuitem_t *save_menuitem= NULL;;
rtk_menuitem_t *exit_menuitem= NULL;;

// The stills menu
rtk_menu_t *stills_menu;
rtk_menuitem_t *stills_jpeg_menuitem= NULL;;
rtk_menuitem_t *stills_ppm_menuitem= NULL;;

// Export stills info
int stills_series;
int stills_count;

// The movie menu  
rtk_menu_t *movie_menu= NULL;;
struct CMovieOption
{
  rtk_menuitem_t *menuitem;
  int speed;
};
int movie_option_count;
CMovieOption movie_options[10];

// Export movie info
int movie_count;

// The view menu
rtk_menu_t *view_menu= NULL;;
rtk_menuitem_t *grid_item= NULL;;
rtk_menuitem_t *walls_item= NULL;;
rtk_menuitem_t *matrix_item= NULL;;
rtk_menuitem_t *objects_item= NULL;;
rtk_menuitem_t *data_item= NULL;;

// The action menu
rtk_menu_t* action_menu= NULL;;
rtk_menuitem_t* subscribedonly_item= NULL;;
rtk_menuitem_t* autosubscribe_item= NULL;;
int autosubscribe;

typedef struct 
{
  rtk_menu_t *menu;
  rtk_menuitem_t *items[ 1000 ]; // TODO - get rid of this fixed size buffer
} stage_menu_t;

// The view/device menu
stage_menu_t device_menu;

// the view/data menu
stage_menu_t data_menu;


// Number of exported images
int export_count;

// these are internal and can only be called in rtkgui.cc
static void RtkMenuHandling();
static void RtkUpdateMovieMenu();

// allow the GUI to do any startup it needs to, including cmdline parsing
int RtkGuiInit( int argc, char** argv )
{ 
  PRINT_DEBUG( "init_func" );
  return( rtk_init(&argc, &argv) );
}


int RtkGuiCreateApp( void )
{
  app = rtk_app_create();
  canvas = rtk_canvas_create(app);
      
  // Add some menu items
  file_menu = rtk_menu_create(canvas, "File");
  save_menuitem = rtk_menuitem_create(file_menu, "Save", 0);
  stills_menu = rtk_menu_create_sub(file_menu, "Capture stills");
  movie_menu = rtk_menu_create_sub(file_menu, "Capture movie");
  exit_menuitem = rtk_menuitem_create(file_menu, "Exit", 0);
  
  stills_jpeg_menuitem = rtk_menuitem_create(stills_menu, "JPEG format", 1);
  stills_ppm_menuitem = rtk_menuitem_create(stills_menu, "PPM format", 1);
  stills_series = 0;
  stills_count = 0;
  
  movie_options[0].menuitem = rtk_menuitem_create(movie_menu, "Speed x1", 1);
  movie_options[0].speed = 1;
  movie_options[1].menuitem = rtk_menuitem_create(movie_menu, "Speed x2", 1);
  movie_options[1].speed = 2;
  movie_options[2].menuitem = rtk_menuitem_create(movie_menu, "Speed x5", 1);
  movie_options[2].speed = 5;
  movie_options[3].menuitem = rtk_menuitem_create(movie_menu, "Speed x10", 1);
  movie_options[3].speed = 10;
  movie_option_count = 4;
  
  movie_count = 0;
  
  // Create the view menu
  view_menu = rtk_menu_create(canvas, "View");
  // create the view menu items and set their initial checked state
  grid_item = rtk_menuitem_create(view_menu, "Grid", 1);
  matrix_item = rtk_menuitem_create(view_menu, "Matrix", 1);
  objects_item = rtk_menuitem_create(view_menu, "Objects", 1);
  data_item = rtk_menuitem_create(view_menu, "Data", 1);
  
  rtk_menuitem_check(matrix_item, 0);
  rtk_menuitem_check(objects_item, 1);
  rtk_menuitem_check(data_item, 1);
  
  // create the action menu
  action_menu = rtk_menu_create(canvas, "Action");
  subscribedonly_item = rtk_menuitem_create(action_menu, 
					    "Subscribe to all", 1);
  
  /* BROKEN
  //zero the view menus
  memset( &device_menu,0,sizeof(stage_menu_t));
  memset( &data_menu,0,sizeof(stage_menu_t));
  
  // Create the view/device sub menu
  assert( data_menu.menu = 
  rtk_menu_create_sub(view_menu, "Data"));
  
  // Create the view/data sub menu
  assert( device_menu.menu = 
  rtk_menu_create_sub(view_menu, "Object"));
  
  // each device adds itself to the correct view menus in its rtkstartup()
  */

  // Start the gui; dont run in a separate thread and dont let it do
  // its own updates.
  rtk_app_main_init(app);

  return 0; //OK
}

// Initialise the GUI

int RtkGuiLoad( void )
{
  PRINT_DEBUG( "load_func" );

  int width = STG_DEFAULT_WINDOW_WIDTH; // window size in pixels
  int height = STG_DEFAULT_WINDOW_HEIGHT; 

  // (TODO - we could get the sceen size from X if we tried?)
  if (width > 1024)
    width = 1024;
  if (height > 768)
    height = 768;
  
  // get this stuff out of the config eventually
  double scale = 1.0/STG_DEFAULT_PPM;
  double dx, dy;
  double ox, oy;

  // Size in meters
  dx = width * scale;
  dy = height * scale;
  
  // Origin of the canvas
  ox = STG_DEFAULT_WINDOW_XORIGIN + dx/2.0;
  oy = STG_DEFAULT_WINDOW_YORIGIN + dy/2.0;
  
  if( app == NULL ) // we need to create the basic data for the app
      RtkGuiCreateApp(); // this builds the app, canvas, menus, etc
  
  // configure the GUI
  rtk_canvas_size( canvas, width, height );
  rtk_canvas_scale( canvas, scale, scale);
  rtk_canvas_origin( canvas, ox, oy);
  
  // check the menu items appropriately
  rtk_menuitem_check(grid_item, STG_DEFAULT_SHOWGRID );
  rtk_menuitem_check(subscribedonly_item, STG_DEFAULT_SHOWSUBSONLY );
 
  rtk_canvas_render( canvas );

  PRINT_WARN( "canvas render" );
  return 0; // success
}

// called frequently so the GUI can handle events
// the rtk gui is contained inside Stage's main thread, so it does a lot
// of work here.
// (if the GUI has its own thread you may not need to do much in here)
int RtkGuiUpdate( void )
{    
  //PRINT_DEBUG( "rtk update" );

  // the time we last updated the GUI
  static double last_update = 0.0; 
  // the time we last checked that the root fits nicely in the canvas
  static double last_fit = 0.0; 

  // we test to see if these change between calls
  static double canvas_scale = 0.0;
  static double canvas_origin_x = 0.0;
  static double canvas_origin_y = 0.0;
  
  // if we have no GUI data, do nothing
  if( !app || !canvas )
    return 0; 
  
  // Process events
  rtk_app_main_loop( app);
  
  // Refresh the gui at a fixed rate (in simulator time).
  if ( CEntity::simtime - last_update < rtkgui_update_interval )
    return 0;

  last_update = CEntity::simtime;

  // when the root object is smaller than the window, keep it centered
  // and scaled to fit the canvas 
  if( CEntity::simtime - last_fit > rtkgui_fit_interval )
    {
      
      int width, height;
      double xscale, yscale, xorg, yorg;
      
      rtk_canvas_get_size( canvas, &width, &height );
      rtk_canvas_get_scale( canvas, &xscale, &yscale );
      rtk_canvas_get_origin( canvas, &xorg, &yorg );
      
      last_fit = CEntity::simtime;
      
      // calculate the desired scale
      if( xscale > (CEntity::root->size_x * 1.1) / width )
	xscale = (CEntity::root->size_x * 1.1) / width;
      
      if( yscale > (CEntity::root->size_y * 1.1) / height )
	yscale = (CEntity::root->size_y * 1.1 ) / height;
      
      // choose the largest scale so we keep the correct aspect ratio
      double scale;
      xscale > yscale ? scale = xscale : scale = yscale; 

      // if we're not at the desired scale, we set it
      if( canvas_scale != scale )
	{
	  rtk_canvas_scale( canvas, scale, scale );
	  canvas_scale = scale;
	}
      
      // calculate the ideal canvas origin
      if( CEntity::root->size_x  < width * xscale ) 
	xorg = CEntity::root->size_x/2.0;
      
      if( CEntity::root->size_y  < height * yscale )
	yorg = CEntity::root->size_y/2.0;
      
      // if we're not at the desired origin, we set it 
      if( canvas_origin_x != xorg || canvas_origin_y != yorg )
	{
	  rtk_canvas_origin( canvas, xorg, yorg );
	  canvas_origin_x = xorg;
	  canvas_origin_y = yorg;
	}
      
      // I'd like to automatically scale the canvas when the user resizes
      // the window, but I can't get those events out of RTK and I don't
      // want to check *everything* each time - rtv
    }

  // Process menus
  RtkMenuHandling();      

  // process any flashers 
  rtk_canvas_flash_update( canvas );

  printf(","), fflush(stdout);
  // Render the canvas
  rtk_canvas_render( canvas);
  
  return 0; // success
}


void UnrenderMatrix()
{
  if( matrix_fig )
    {
      rtk_fig_destroy( matrix_fig );
      matrix_fig = NULL;
    }
}

void RenderMatrix( )
{
  assert( CEntity::matrix );

  // Create a figure representing this object
  if( matrix_fig )
    UnrenderMatrix();
  
  matrix_fig = rtk_fig_create( canvas, NULL, 60);
  
  // Set the default color 
  rtk_fig_color_rgb32( matrix_fig, ::LookupColor(MATRIX_COLOR) );
  
  double pixel_size = 1.0 / CEntity::matrix->ppm;
  
  // render every pixel as an unfilled rectangle
  for( int y=0; y<CEntity::matrix->get_height(); y++ )
    for( int x=0; x<CEntity::matrix->get_width(); x++ )
      if( *(CEntity::matrix->get_cell( x, y )) )
	rtk_fig_rectangle( matrix_fig, x*pixel_size, y*pixel_size, 0, pixel_size, pixel_size, 0 );
}


int RtkGuiEntityPropertyChange( CEntity* ent, stage_prop_id_t prop )
{
  //PRINT_DEBUG2( "setting prop %d on ent %s", (prop), ent->name );

  assert(ent);
  assert( prop < STG_PROPERTY_COUNT );
  
  // if it has no fig, do nothing
  if( !ent->fig )
    return 0; 
  
  double px, py, pa;
  
  switch( prop )
    {
      // these require just moving the figure
    case STG_PROP_ENTITY_POSE:
      ent->GetPose( px, py, pa );
      //PRINT_DEBUG3( "moving figure to %.2f %.2f %.2f", px,py,pa );
      rtk_fig_origin(ent->fig, px, py, pa );
      break;

    case STG_PROP_ENTITY_ORIGIN:
      // PRINT_WARN2( "entity %s moved to %.2f %.2f", ent->name, 
      // these all need us to totally redraw the object
    case STG_PROP_ENTITY_SIZE:
    case STG_PROP_ENTITY_PARENT:
    case STG_PROP_ENTITY_NAME:
    case STG_PROP_ENTITY_COLOR:
    case STG_PROP_ENTITY_LASERRETURN:
    case STG_PROP_ENTITY_SONARRETURN:
    case STG_PROP_ENTITY_IDARRETURN:
    case STG_PROP_ENTITY_OBSTACLERETURN:
    case STG_PROP_ENTITY_VISIONRETURN:
    case STG_PROP_ENTITY_PUCKRETURN:
    case STG_PROP_ENTITY_PLAYERID:
    case STG_PROP_ENTITY_RECTS:
    case STG_PROP_ENTITY_CIRCLES:
    case STG_PROP_ENTITY_VELOCITY:
      ent->RtkShutdown(); 
      ent->RtkStartup(canvas);
      break;

      // 
    case STG_PROP_ENTITY_DATA:
    case STG_PROP_ENTITY_COMMAND:
      //PRINT_DEBUG1( "gui redraws data", prop ); 
      ent->RtkUpdate(); 
      break;      

      /*
    case STG_PROP_IDAR_RX:
      //PRINT_ERR1( "rtk IDAR_RX for idar %d", ent->name );
      ((CIdarModel*)ent)->RtkShowReceived();
      break;
      
    case STG_PROP_IDAR_TX:
      //PRINT_ERR1( "rtk IDAR_TX for idar %d", ent->name );
      ((CIdarModel*)ent)->RtkShowSent();
      break;
      */

    case STG_PROP_ENTITY_SUBSCRIBE:
      ent->RtkUpdate(); 
      break;

    default:
      //PRINT_WARN1( "property change %s unhandled by rtkgui", SIOPropString(prop) ); 
      break;
    } 
  
  return 0;
}
 


// END OF INTERFACE FUNCTIONS //////////////////////////////////////////

void AddToMenu( stage_menu_t* menu, CEntity* ent, int check )
{
  /* BROKEN
  assert( menu );
  assert( ent );

  // if there's no menu item for this type yet
  if( menu->items[ ent->lib_entry->type_num ] == NULL )
    // create a new menu item
    assert( menu->items[ ent->lib_entry->type_num ] =  
	    rtk_menuitem_create( menu->menu, ent->lib_entry->token, 1 ));
  
  rtk_menuitem_check( menu->items[ ent->lib_entry->type_num ], check );
  */
}


void AddToDataMenu(  CEntity* ent, int check )
{
  /* BROKEN
  assert( ent );
  AddToMenu( &this->data_menu, ent, check );
  */
}

void AddToDeviceMenu(  CEntity* ent, int check )
{
  /* BROKEN
  assert( ent );
  AddToMenu( &this->device_menu, ent, check );
  */
}

// devices check this to see if they should display their data
bool ShowDeviceData( int devtype )
{
  //return rtk_menuitem_ischecked(this->data_item);
  
  /* BROKEN
  rtk_menuitem_t* menu_item = data_menu.items[ devtype ];
  
  if( menu_item )
    return( rtk_menuitem_ischecked( menu_item ) );  
  else // if there's no option in the menu, display this data
    return true;
  */

  return true;
}

bool ShowDeviceBody( int devtype )
{
  //return rtk_menuitem_ischecked(this->objects_item);

  /* BROKEN
  rtk_menuitem_t* menu_item = device_menu.items[ devtype ];
  
  if( menu_item )
    return( rtk_menuitem_ischecked( menu_item ) );  
  else // if there's no option in the menu, display this data
    return true;
  */
  return true;
}



// Update the GUI
void RtkMenuHandling()
{
  char filename[128];
  
  // See if we need to quit the program
  if (rtk_menuitem_isactivated(exit_menuitem))
    ::quit = 1;
  if (rtk_canvas_isclosed(canvas))
    ::quit = 1;

  // Save the world file
  //if (rtk_menuitem_isactivated(save_menuitem))
  //Save();
  
  // Start/stop export (jpeg)
  if (rtk_menuitem_isactivated(stills_jpeg_menuitem))
  {
    if (rtk_menuitem_ischecked(stills_jpeg_menuitem))
    {
      stills_series++;
      rtk_menuitem_enable(stills_ppm_menuitem, 0);
    }
    else
      rtk_menuitem_enable(stills_ppm_menuitem, 1);      
  }
  if (rtk_menuitem_ischecked(stills_jpeg_menuitem))
  {
    snprintf(filename, sizeof(filename), "stage-%03d-%04d.jpg",
             stills_series, stills_count++);
    printf("saving [%s]\n", filename);
    rtk_canvas_export_image(canvas, filename, RTK_IMAGE_FORMAT_JPEG);
  }

  // Start/stop export (ppm)
  if (rtk_menuitem_isactivated(stills_ppm_menuitem))
  {
    if (rtk_menuitem_ischecked(stills_ppm_menuitem))
    {
      stills_series++;
      rtk_menuitem_enable(stills_jpeg_menuitem, 0);
    }
    else
      rtk_menuitem_enable(stills_jpeg_menuitem, 1);      
  }
  if (rtk_menuitem_ischecked(stills_ppm_menuitem))
  {
    snprintf(filename, sizeof(filename), "stage-%03d-%04d.ppm",
             stills_series, stills_count++);
    printf("saving [%s]\n", filename);
    rtk_canvas_export_image(canvas, filename, RTK_IMAGE_FORMAT_PPM);
  }


  // Update movie menu
  RtkUpdateMovieMenu();
  
  // Show or hide the grid
  //if (rtk_menuitem_ischecked(grid_item))
  //rtk_fig_show( CEntity::rootfig_grid, 1);
  //else
  //rtk_fig_show(fig_grid, 0);

  // clear any matrix rendering, then redraw if the emnu item is checked
  //matrix->unrender();
  if (rtk_menuitem_ischecked(matrix_item))
    RenderMatrix();
  else
    if( matrix_fig ) UnrenderMatrix();
  
  // enable/disable subscriptions to show sensor data
  //static bool lasttime = rtk_menuitem_ischecked(subscribedonly_item);
  //bool thistime = rtk_menuitem_ischecked(subscribedonly_item);

  // for now i check if the menu item changed
  /*
    if( thistime != lasttime )
    {
    if( thistime )  // change the subscription counts of any player-capable ent
    CEntity::root->FamilySubscribe();
    else
    CEntity::root->FamilyUnsubscribe();
    // remember this state
    lasttime = thistime;
    }
  */
  return;
}


// Handle the movie sub-menu.
// This is still yucky.
void RtkUpdateMovieMenu()
{
  int i, j;
  char filename[128];
  CMovieOption *option;
  
  for (i = 0; i < movie_option_count; i++)
  {
    option = movie_options + i;
    
    if (rtk_menuitem_isactivated(option->menuitem))
    {
      if (rtk_menuitem_ischecked(option->menuitem))
      {
        // Start the movie capture
        snprintf(filename, sizeof(filename), "stage-%03d-sp%02d.mpg",
                 movie_count++, option->speed);
        rtk_canvas_movie_start(canvas, filename,
                               1.0/rtkgui_update_interval, option->speed);

        // Disable all other capture options
        for (j = 0; j < movie_option_count; j++)
          rtk_menuitem_enable(movie_options[j].menuitem, i == j);
      }
      else
      {
        // Stop movie capture
        rtk_canvas_movie_stop(canvas);

        // Enable all capture options
        for (j = 0; j < movie_option_count; j++)
          rtk_menuitem_enable(movie_options[j].menuitem, 1);
      }
    }

    // Export the frame
    if (rtk_menuitem_ischecked(option->menuitem))
      rtk_canvas_movie_frame(canvas);
  }
  return;
}

///////////////////////////////////////////////////////////////////////////
// Process mouse events 
void RtkOnMouse(rtk_fig_t *fig, int event, int mode)
{
  CEntity *entity = (CEntity*) fig->userdata;
  
  static rtk_fig_t* fig_pose = NULL;

  double px, py, pth;
  
  switch (event)
    {
    case RTK_EVENT_PRESS:

      PRINT_WARN( "MOUSE PRESS" );
      
      rtk_fig_show( entity->fig_label, 1 );

      if( fig_pose ) rtk_fig_destroy( fig_pose );
      fig_pose = rtk_fig_create( canvas, entity->fig_label, 51 );
      rtk_fig_color_rgb32( fig_pose, 0x000000 );
      
      // DELIBERATE NO BREAK
      
    case RTK_EVENT_MOTION:
      rtk_fig_get_origin(fig, &px, &py, &pth);
      entity->SetGlobalPose(px, py, pth);
      
      rtk_fig_origin( entity->fig_label, 
		      px + entity->size_x,
		      py + entity->size_y, 0.0 );
      
      // display the pose
      char txt[100];
      txt[0] = 0;
      snprintf(txt, sizeof(txt), "[%.2f,%.2f,%.2f]",  px,py,pth  );
      rtk_fig_clear( fig_pose );
      rtk_fig_text( fig_pose, 0.2, -0.3, 0.0, txt );
      break;
      

    case RTK_EVENT_RELEASE:
      rtk_fig_show( entity->fig_label, 0 );

      rtk_fig_get_origin(fig, &px, &py, &pth);
      entity->SetGlobalPose(px, py, pth);
      
      rtk_fig_destroy( fig_pose );
      fig_pose = NULL;
      break;
      
      
    default:
      break;
    }
  return;
}


#endif


