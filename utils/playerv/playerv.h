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

#define COLOR_LASER_SCAN         0x0000C0
#define COLOR_LASERBEACON_BEACON 0x0000C0



/***************************************************************************
 * Top-level GUI elements
 ***************************************************************************/

// Window containing the sensor data.
typedef struct
{
  // The rtk canvas
  rtk_canvas_t *canvas;

  // The base figure for the robot
  rtk_fig_t *robot_fig;

  // Menu containing file options
  rtk_menu_t *file_menu;
  rtk_menuitem_t *exit_item;

  // Menu containing the device list
  rtk_menu_t *device_menu;
  
} mainwnd_t;


// Create the image window
mainwnd_t *mainwnd_create(rtk_app_t *app);

// Destroy the main window
void mainwnd_destroy(mainwnd_t *wnd);

// Update the window
void mainwnd_update(mainwnd_t *wnd);


// Window containing tabular data.
typedef struct
{
  // The RTK table widget
  rtk_table_t *table;   

} tablewnd_t;

// Create the table window
tablewnd_t *tablewnd_init(rtk_app_t *app);


/***************************************************************************
 * Laser device
 ***************************************************************************/

// Laser device info
typedef struct
{
  // Subscribe/unsibscribe menu item
  rtk_menuitem_t *menuitem;

  // Figure for drawing the laser scan
  rtk_fig_t *scan_fig;
  
  // Laser device proxy
  playerc_laser_t *proxy;

  // Non-zero if the laser is currently subscribed
  int subscribed;

  // Timestamp on most recent data
  double datatime;
  
} laser_t;


// Create a laser device
laser_t *laser_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index);

// Destroy a laser device
void laser_destroy(laser_t *laser);

// Update a laser device
void laser_update(laser_t *laser);


/***************************************************************************
 * Laser beacon device
 ***************************************************************************/

// LaserBeacon device info
typedef struct
{
  // Menu
  rtk_menu_t *menu;
  rtk_menuitem_t *subscribe_item;
  rtk_menuitem_t *bits5_item, *bits8_item;

  // Figure for drawing the beacons
  rtk_fig_t *beacon_fig;
  
  // LaserBeacon device proxy
  playerc_laserbeacon_t *proxy;

  // Non-zero if the laser is currently subscribed
  int subscribed;

  // Timestamp on most recent data
  double datatime;
  
} laserbeacon_t;


// Create a laserbeacon device
laserbeacon_t *laserbeacon_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                                  int index);

// Destroy a laserbeacon device
void laserbeacon_destroy(laserbeacon_t *laserbeacon);

// Update a laserbeacon device
void laserbeacon_update(laserbeacon_t *laserbeacon);



#endif
