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
mainwnd_t *mainwnd_create(rtk_app_t *app, const char *host, int port)
{
  char title[128];
  mainwnd_t *wnd;

  wnd = malloc(sizeof(mainwnd_t));
  wnd->canvas = rtk_canvas_create(app);

  wnd->host = host;
  wnd->port = port;

  // Set up the canvas
  rtk_canvas_movemask(wnd->canvas, RTK_MOVE_PAN | RTK_MOVE_ZOOM);
  rtk_canvas_size(wnd->canvas, 320, 240);
  rtk_canvas_scale(wnd->canvas, 0.02, 0.02);
  rtk_canvas_origin(wnd->canvas, 0, 0);

  snprintf(title, sizeof(title), "PlayerViewer %s:%d (main)", host, port);
  rtk_canvas_title(wnd->canvas, title);

  // Create file menu
  wnd->file_menu = rtk_menu_create(wnd->canvas, "File");
  wnd->stills_item = rtk_menuitem_create(wnd->file_menu, "Export stills", 1);
  wnd->exit_item = rtk_menuitem_create(wnd->file_menu, "Exit", 0);

  wnd->stills_count = 0;
  
  // Create view menu
  wnd->view_menu = rtk_menu_create(wnd->canvas, "View");
  wnd->view_item_rotate = rtk_menuitem_create(wnd->view_menu, "Rotate", 1);
  wnd->view_item_1m = rtk_menuitem_create(wnd->view_menu, "Grid 1 m", 1);
  wnd->view_item_2f = rtk_menuitem_create(wnd->view_menu, "Grid 2 feet", 1);

  // Create device menu
  wnd->device_menu = rtk_menu_create(wnd->canvas, "Devices");

  // Create figure to draw the grid on
  wnd->grid_fig = rtk_fig_create(wnd->canvas, NULL, -99);
    
  // Create a figure to attach everything else to
  wnd->robot_fig = rtk_fig_create(wnd->canvas, NULL, 0);

  // Set the initial grid state (this is a bit of a hack, since
  // it duplicated the code in update()).
  rtk_menuitem_check(wnd->view_item_rotate, 0);
  rtk_menuitem_check(wnd->view_item_1m, 1);
  rtk_menuitem_check(wnd->view_item_2f, 0);
  rtk_fig_color_rgb32(wnd->grid_fig, COLOR_GRID_MINOR);
  rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 0.2);
  rtk_fig_color_rgb32(wnd->grid_fig, COLOR_GRID_MAJOR);
  rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 1);
      
  return wnd;
}


// Destroy the main window
void mainwnd_destroy(mainwnd_t *wnd)
{
  // Destroy the grid menu
  rtk_menuitem_destroy(wnd->view_item_rotate);
  rtk_menuitem_destroy(wnd->view_item_1m);
  rtk_menuitem_destroy(wnd->view_item_2f);
  rtk_menu_destroy(wnd->view_menu);
  
  // Destroy device menu
  rtk_menu_destroy(wnd->device_menu);

  // Destroy file menu
  rtk_menuitem_destroy(wnd->exit_item);
  rtk_menu_destroy(wnd->file_menu);

  // Destroy canvas
  rtk_fig_destroy(wnd->robot_fig);
  rtk_fig_destroy(wnd->grid_fig);
  rtk_canvas_destroy(wnd->canvas);
  
  free(wnd);
}


// Update the window.
// Returns 1 if the program should quit.
int mainwnd_update(mainwnd_t *wnd)
{
  char filename[256];
  
  // See if we should quit
  if (rtk_canvas_isclosed(wnd->canvas))
    return 1;
  if (rtk_menuitem_isactivated(wnd->exit_item))
    return 1;

  // Export stills.
  if (rtk_menuitem_ischecked(wnd->stills_item))
  {
    snprintf(filename, sizeof(filename), "playerv-%s-%d-%04d.jpg",
             wnd->host, wnd->port, wnd->stills_count++);
    printf("exporting %s\n", filename);
    rtk_canvas_export_jpeg(wnd->canvas, filename);
  }

  // Rotate the display
  if (rtk_menuitem_isactivated(wnd->view_item_rotate))
  {
    if (rtk_menuitem_ischecked(wnd->view_item_rotate))
      rtk_fig_origin(wnd->robot_fig, 0, 0, M_PI / 2);
    else
      rtk_fig_origin(wnd->robot_fig, 0, 0, 0);
  }
  
  // Draw in the grid, perhaps
  if (rtk_menuitem_isactivated(wnd->view_item_1m))
  {
    rtk_fig_clear(wnd->grid_fig);
    if (rtk_menuitem_ischecked(wnd->view_item_1m))
    {
      rtk_fig_color_rgb32(wnd->grid_fig, COLOR_GRID_MINOR);
      rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 0.2);
      rtk_fig_color_rgb32(wnd->grid_fig, COLOR_GRID_MAJOR);
      rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 1);
      rtk_menuitem_check(wnd->view_item_2f, 0);
    }
  }

  // Draw in the grid, perhaps
  if (rtk_menuitem_isactivated(wnd->view_item_2f))
  {
    rtk_fig_clear(wnd->grid_fig);
    if (rtk_menuitem_ischecked(wnd->view_item_2f))
    {
      rtk_fig_color_rgb32(wnd->grid_fig, COLOR_GRID_MINOR);
      rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 4 * 0.0254);
      rtk_fig_color_rgb32(wnd->grid_fig, COLOR_GRID_MAJOR);
      rtk_fig_grid(wnd->grid_fig, 0, 0, 50, 50, 2 * 12 * 0.0254);
      rtk_menuitem_check(wnd->view_item_1m, 0);
    }
  }

  // Render the canvas
  rtk_canvas_render(wnd->canvas);
        
  return 0;
}







