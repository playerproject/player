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


// Create the image window
imagewnd_t *imagewnd_create(rtk_app_t *app, const char *host, int port)
{
  char title[128];
  imagewnd_t *wnd;

  wnd = malloc(sizeof(imagewnd_t));
  wnd->canvas = rtk_canvas_create(app);

  // Set up the canvas
  rtk_canvas_size(wnd->canvas, 256, 256);
  rtk_canvas_scale(wnd->canvas, 1, -1);
  rtk_canvas_origin(wnd->canvas, 128, 128);

  snprintf(title, sizeof(title), "PlayerViewer %s:%d (image)", host, port);
  rtk_canvas_title(wnd->canvas, title);

  return wnd;
}


// Destroy the image window
void imagewnd_destroy(imagewnd_t *wnd)
{
  // Destroy canvas
  rtk_canvas_destroy(wnd->canvas);
  
  free(wnd);
}


// Update the window.
// Returns 1 if the program should quit.
int imagewnd_update(imagewnd_t *wnd)
{
  return 0;
}







