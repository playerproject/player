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
 * Desc: Laser device interface
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "playerv.h"


// Draw the laser scan
void laser_draw(laser_t *laser);


// Create a laser device
laser_t *laser_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index)
{
  char label[64];
  laser_t *laser;
  
  laser = malloc(sizeof(laser_t));

  snprintf(label, sizeof(label), "Laser %d", index);
  laser->menuitem = rtk_menuitem_create(mainwnd->device_menu, label, 1);
  laser->scan_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);
  
  laser->proxy = playerc_laser_create(client, index);
  laser->subscribed = 0;
  laser->datatime = 0;

  return laser;
}


// Destroy a laser device
void laser_destroy(laser_t *laser)
{
  if (laser->subscribed)
    playerc_laser_unsubscribe(laser->proxy);
  playerc_laser_destroy(laser->proxy);

  rtk_fig_destroy(laser->scan_fig);
  rtk_menuitem_destroy(laser->menuitem);
  free(laser);
}


// Update a laser device
void laser_update(laser_t *laser)
{
  // See if the subscription menu item has changed
  if (rtk_menuitem_ischecked(laser->menuitem) != laser->subscribed)
  {
    if (!laser->subscribed)
    {
      if (playerc_laser_subscribe(laser->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
      else
        laser->subscribed = 1;
    }
    else
    {
      if (playerc_laser_unsubscribe(laser->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
      else
        laser->subscribed = 0;
    }
    rtk_menuitem_check(laser->menuitem, laser->subscribed);
  }

  if (laser->subscribed)
  {
    // Draw in the laser scan if it has been changed.
    if (laser->proxy->info.datatime != laser->datatime)
    {
      laser_draw(laser);
      laser->datatime = laser->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the laser.
    rtk_fig_show(laser->scan_fig, 0);
  }
}


// Draw the laser scan
void laser_draw(laser_t *laser)
{
  int i;
  double ax, ay, bx, by;

  rtk_fig_show(laser->scan_fig, 1);      
  rtk_fig_clear(laser->scan_fig);
  rtk_fig_color_rgb32(laser->scan_fig, COLOR_LASER_SCAN);

  rtk_fig_rectangle(laser->scan_fig, 0, 0, 0, 0.15, 0.15, 0);
  
  for (i = 0; i < laser->proxy->scan_count; i++)
  {
    bx = laser->proxy->point[i][0];
    by = laser->proxy->point[i][1];
    if (i == 0)
    {
      ax = bx;
      ay = by;
    }
    rtk_fig_line(laser->scan_fig, ax, ay, bx, by);
    ax = bx;
    ay = by;
  }
}




