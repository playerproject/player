/* 
 *  PlayerViewer
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Public strutures, functions
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#ifndef PLAYERV_H
#define PLAYERV_H

// Non-local headers
#include "rtk.h"
#include "playerc.h"

// Local headers
#include "error.h"
#include "opt.h"



/***************************************************************************
 * Default colors
 ***************************************************************************/

#define COLOR_GRID_MAJOR         0xC0C0C0
#define COLOR_GRID_MINOR         0xE0E0E0
#define COLOR_LASER              0x0000C0
#define COLOR_LASER_EMP          0xD0D0FF
#define COLOR_LASER_OCC          0x0000C0
#define COLOR_FIDUCIAL           0x0000C0
#define COLOR_POSITION_ROBOT     0xC00000
#define COLOR_POSITION_CONTROL   0xFF0000
#define COLOR_PTZ_DATA           0x00C000
#define COLOR_PTZ_CMD            0x00C000
#define COLOR_SONAR              0xC0C080
#define COLOR_SONAR_SCAN         0xC0C080


/***************************************************************************
 * Top-level GUI elements
 ***************************************************************************/

// Main window displaying sensor stuff
typedef struct
{
  const char *host;
  int port;
  
  // The rtk canvas
  rtk_canvas_t *canvas;
  
  // The base figure for the robot
  rtk_fig_t *grid_fig;
  rtk_fig_t *robot_fig;
  
  // Menu containing file options
  rtk_menu_t *file_menu;
  rtk_menuitem_t *stills_item;
  rtk_menuitem_t *exit_item;

  // Menu containing view settings
  rtk_menu_t *view_menu;
  rtk_menuitem_t *view_item_rotate;
  rtk_menuitem_t *view_item_1m;
  rtk_menuitem_t *view_item_2f;
  
  // Menu containing the device list
  rtk_menu_t *device_menu;

  // Export info.
  int stills_count;
  
} mainwnd_t;


// Create the main window
mainwnd_t *mainwnd_create(rtk_app_t *app, const char *host, int port);

// Destroy the main window
void mainwnd_destroy(mainwnd_t *wnd);

// Update the window
// Returns 1 if the program should quit.
int mainwnd_update(mainwnd_t *wnd);


/***************************************************************************
 * Device registry
 ***************************************************************************/

// Callback prototypes
typedef void (*fndestroy_t) (void*);
typedef void (*fnupdate_t) (void*);


// Somewhere to store which devices are available.
typedef struct
{
  // Device identifier.
  int code, index;
  char drivername[PLAYER_MAX_DEVICE_STRING_LEN];
  
  // Handle to the GUI proxy for this device.
  void *proxy;

  // Callbacks
  fndestroy_t fndestroy;
  fnupdate_t fnupdate;

  // Non-zero if should be subscribed.
  int subscribe;
  
} device_t;

// Create the appropriate GUI proxy for a given set of device info.
void create_proxy(device_t *device, opt_t *opt,
                  mainwnd_t *mainwnd, playerc_client_t *client);


/***************************************************************************
 * Laser (scanning range-finder)
 ***************************************************************************/

// Laser device info
typedef struct
{
  // Menu stuff
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;
  rtk_menuitem_t *res025_item, *res050_item, *res100_item;

  // Figure for drawing the scan
  rtk_fig_t *scan_fig;
  
  // Laser device proxy
  playerc_laser_t *proxy;

  // Timestamp on most recent data
  double datatime;
  
} laser_t;


// Create a laser device
laser_t *laser_create(mainwnd_t *mainwnd, opt_t *opt,
                  playerc_client_t *client, int index, int subscribe);

// Destroy a laser device
void laser_destroy(laser_t *laser);

// Update a laser device
void laser_update(laser_t *laser);


/***************************************************************************
 * Fiducial detector
 ***************************************************************************/

// Fiducial detector info
typedef struct
{
  // Menu stuff
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;
  rtk_menuitem_t *bits5_item, *bits8_item;

  // Figure for drawing the fiducials.
  rtk_fig_t *fig;
  
  // Fiducial device proxy
  playerc_fiducial_t *proxy;

  // Timestamp on most recent data
  double datatime;
  
} fiducial_t;


// Create a fiducial device
fiducial_t *fiducial_create(mainwnd_t *mainwnd, opt_t *opt,
                            playerc_client_t *client, int index, int subscribe);

// Destroy a fiducial device
void fiducial_destroy(fiducial_t *fiducial);

// Update a fiducial device
void fiducial_update(fiducial_t *fiducial);


/***************************************************************************
 * Position device
 ***************************************************************************/

// Position device info
typedef struct
{
  // Menu stuff
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;
  rtk_menuitem_t *command_item;
  rtk_menuitem_t *enable_item, *disable_item;

  // Figures
  rtk_fig_t *robot_fig;
  rtk_fig_t *control_fig;
  rtk_fig_t *path_fig;
  
  // Position device proxy
  playerc_position_t *proxy;

  // Timestamp on most recent data
  double datatime;

} position_t;


// Create a position device
position_t *position_create(mainwnd_t *mainwnd, opt_t *opt,
                            playerc_client_t *client, int index, int subscribe);

// Destroy a position device
void position_destroy(position_t *position);

// Update a position device
void position_update(position_t *position);


/***************************************************************************
 * PTZ (pan-tilt-zoom) device
 ***************************************************************************/

// PTZ device info
typedef struct
{
  // Ptz device proxy
  playerc_ptz_t *proxy;

  // Menu stuff
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;
  rtk_menuitem_t *command_item;

  // Figures
  rtk_fig_t *data_fig;
  rtk_fig_t *cmd_fig;
  
  // Timestamp on most recent data
  double datatime;
  
} ptz_t;


// Create a ptz device
ptz_t *ptz_create(mainwnd_t *mainwnd, opt_t *opt,
                  playerc_client_t *client, int index, int subscribe);

// Destroy a ptz device
void ptz_destroy(ptz_t *ptz);

// Update a ptz device
void ptz_update(ptz_t *ptz);


/***************************************************************************
 * Sonar (Fixed range-finder) device
 ***************************************************************************/

// SONAR device info
typedef struct
{
  // Menu stuff
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;

  // Figures for drawing the sonar scan
  rtk_fig_t *scan_fig[PLAYERC_SONAR_MAX_SAMPLES];
  
  // Sonar device proxy
  playerc_sonar_t *proxy;

  // Timestamp on most recent data
  double datatime;
  
} sonar_t;


// Create a sonar device
sonar_t *sonar_create(mainwnd_t *mainwnd, opt_t *opt,
                  playerc_client_t *client, int index, int subscribe);

// Destroy a sonar device
void sonar_destroy(sonar_t *sonar);

// Update a sonar device
void sonar_update(sonar_t *sonar);


/***************************************************************************
 * Blobfinder device
 ***************************************************************************/

// Blobfinder device info
typedef struct
{
  // Blobfinder device proxy
  playerc_blobfinder_t *proxy;

  // Menu stuff
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;
  rtk_menuitem_t *stats_item;

  // Figure for drawing the blobfinder scan
  rtk_fig_t *image_fig;
  int image_init;

  // Image scale (m/pixel)
  double scale;
  
  // Timestamp on most recent data
  double datatime;
  
} blobfinder_t;


// Create a blobfinder device
blobfinder_t *blobfinder_create(mainwnd_t *mainwnd, opt_t *opt,
                                playerc_client_t *client, int index, int subscribe);

// Destroy a blobfinder device
void blobfinder_destroy(blobfinder_t *blobfinder);

// Update a blobfinder device
void blobfinder_update(blobfinder_t *blobfinder);



#endif
