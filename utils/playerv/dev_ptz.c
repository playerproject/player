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
 * Desc: PTZ device interface
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "playerv.h"


// Draw the ptz 
void ptz_draw(ptz_t *ptz);

// Move the ptz
void ptz_move(ptz_t *ptz);

// Camera field of view at min and max zoom
// TODO: get these from somewhere
#define FMIN (6 * M_PI / 180)
#define FMAX (60 * M_PI / 180)


// Create a ptz device
ptz_t *ptz_create(mainwnd_t *mainwnd, opt_t *opt,
                  playerc_client_t *client, int index)
{
  int subscribe;
  char section[64];
  char label[64];
  ptz_t *ptz;
  
  ptz = malloc(sizeof(ptz_t));
  ptz->datatime = 0;
  ptz->proxy = playerc_ptz_create(client, index);

  // Set initial device state
  snprintf(section, sizeof(section), "ptz:%d", index);
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  if (subscribe)
  {
    if (playerc_ptz_subscribe(ptz->proxy, PLAYER_ALL_MODE) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  // Construct the menu
  snprintf(label, sizeof(label), "ptz %d", index);
  ptz->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  ptz->subscribe_item = rtk_menuitem_create(ptz->menu, "Subscribe", 1);
  ptz->command_item = rtk_menuitem_create(ptz->menu, "Command", 1);
  
  // Set the initial menu state
  rtk_menuitem_check(ptz->subscribe_item, ptz->proxy->info.subscribed);

  // Construct figures
  ptz->data_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 0);
  ptz->cmd_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);
  rtk_fig_movemask(ptz->cmd_fig, RTK_MOVE_TRANS);
  rtk_fig_origin(ptz->cmd_fig, 1, 0, 0);
  rtk_fig_color_rgb32(ptz->cmd_fig, COLOR_PTZ_CMD);
  rtk_fig_ellipse(ptz->cmd_fig, 0, 0, 0, 0.2, 0.2, 0);
  
  return ptz;
}


// Destroy a ptz device
void ptz_destroy(ptz_t *ptz)
{
  // Destroy figures
  rtk_fig_destroy(ptz->cmd_fig);
  rtk_fig_destroy(ptz->data_fig);

  // Destroy menu items
  rtk_menuitem_destroy(ptz->subscribe_item);
  rtk_menu_destroy(ptz->menu);

  // Unsubscribe/destroy the proxy
  if (ptz->proxy->info.subscribed)
    playerc_ptz_unsubscribe(ptz->proxy);
  playerc_ptz_destroy(ptz->proxy);
  
  free(ptz);
}


// Update a ptz device
void ptz_update(ptz_t *ptz)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(ptz->subscribe_item))
  {
    if (!ptz->proxy->info.subscribed)
      if (playerc_ptz_subscribe(ptz->proxy, PLAYER_ALL_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (ptz->proxy->info.subscribed)
      if (playerc_ptz_unsubscribe(ptz->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(ptz->subscribe_item, ptz->proxy->info.subscribed);

  // Draw in the ptz scan if it has been changed.
  if (ptz->proxy->info.subscribed)
  {
    if (ptz->proxy->info.datatime != ptz->datatime)
      ptz_draw(ptz);
    ptz->datatime = ptz->proxy->info.datatime;
  }
  else
  {
    rtk_fig_show(ptz->data_fig, 0);

  }

  // Move the ptz
  if (ptz->proxy->info.subscribed && rtk_menuitem_ischecked(ptz->command_item))
  {
    rtk_fig_show(ptz->cmd_fig, 1);
    ptz_move(ptz);
  }
  else
    rtk_fig_show(ptz->cmd_fig, 0);
}


// Draw the ptz scan
void ptz_draw(ptz_t *ptz)
{
  double ox, oy, d;
  double ax, ay, bx, by;
  double fx, fd;

  // Camera field of view in x-direction (radians)
  fx = FMAX + (double) ptz->proxy->zoom / 1024 * (FMIN - FMAX);
  fd = 1.0 / tan(fx/2);
  
  rtk_fig_show(ptz->data_fig, 1);      
  rtk_fig_clear(ptz->data_fig);

  rtk_fig_color_rgb32(ptz->data_fig, COLOR_PTZ_DATA);
  ox = 100 * cos(ptz->proxy->pan);
  oy = 100 * sin(ptz->proxy->pan);
  rtk_fig_line(ptz->data_fig, 0, 0, ox, oy);
  ox = 100 * cos(ptz->proxy->pan + fx / 2);
  oy = 100 * sin(ptz->proxy->pan + fx / 2);
  rtk_fig_line(ptz->data_fig, 0, 0, ox, oy);
  ox = 100 * cos(ptz->proxy->pan - fx / 2);
  oy = 100 * sin(ptz->proxy->pan - fx / 2);
  rtk_fig_line(ptz->data_fig, 0, 0, ox, oy);

  // Draw in the zoom bar (2 m in length)
  d = sqrt(fd * fd + 1.0 * 1.0);
  ax = d * cos(ptz->proxy->pan + fx / 2);
  ay = d * sin(ptz->proxy->pan + fx / 2);
  bx = d * cos(ptz->proxy->pan - fx / 2);
  by = d * sin(ptz->proxy->pan - fx / 2);
  rtk_fig_line(ptz->data_fig, ax, ay, bx, by);
}


// Move the ptz
void ptz_move(ptz_t *ptz)
{
  double ox, oy, oa;
  double pan, tilt, zoom;
  double fx;
    
  rtk_fig_get_origin(ptz->cmd_fig, &ox, &oy, &oa);

  pan = atan2(oy, ox);
  tilt = 0;

  fx = 2 * atan2(1.0, sqrt(ox * ox + oy * oy));
  zoom = 1024 * (fx - FMAX) / (FMIN - FMAX);
  //zoom = 1024 * sqrt(ox * ox + oy * oy) / 4.0;
  
  if (playerc_ptz_set(ptz->proxy, pan, tilt, zoom) != 0)
    PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
}




