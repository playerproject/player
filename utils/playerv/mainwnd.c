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
 * Desc: Main window with sensor data
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "playerv.h"


// Create the main window
mainwnd_t *mainwnd_create(rtk_app_t *app)
{
  mainwnd_t *wnd;

  wnd = malloc(sizeof(mainwnd_t));
  wnd->canvas = rtk_canvas_create(app);

  // Set up the canvas
  rtk_canvas_movemask(wnd->canvas, RTK_MOVE_PAN | RTK_MOVE_ZOOM);
  rtk_canvas_size(wnd->canvas, 300, 300);
  rtk_canvas_scale(wnd->canvas, 0.02, 0.02);
  rtk_canvas_origin(wnd->canvas, 0, 0);
  wnd->robot_fig = rtk_fig_create(wnd->canvas, NULL, 0);
  
  // Create file menu
  wnd->file_menu = rtk_menu_create(wnd->canvas, "File");
  wnd->exit_item = rtk_menuitem_create(wnd->file_menu, "Exit", 0);

  // Create device menu
  wnd->device_menu = rtk_menu_create(wnd->canvas, "Devices");

  return wnd;
}


// Destroy the main window
void mainwnd_destroy(mainwnd_t *wnd)
{
  // Destroy device menu
  rtk_menu_destroy(wnd->device_menu);

  // Destroy file menu
  rtk_menuitem_destroy(wnd->exit_item);
  rtk_menu_destroy(wnd->file_menu);

  // Destroy canvas
  rtk_fig_destroy(wnd->robot_fig);
  rtk_canvas_destroy(wnd->canvas);
  
  free(wnd);
}


// Update the window
void mainwnd_update(mainwnd_t *wnd)
{

}







