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
  int subscribe;
  double ox, oy, oa;
  char label[64];
  char section[64];
  laser_t *laser;
  
  laser = malloc(sizeof(laser_t));

  laser->proxy = playerc_laser_create(client, index);
  laser->datatime = 0;

  snprintf(section, sizeof(section), "laser:%d", index);

  // Get laser geometry
  // TESTING
  if (index == 1)
  {
    ox = 0;
    oy = 0;
    oa = M_PI;
  }
  else
  {
    ox = 0;
    oy = 0;
    oa = 0;
  }

  // Set initial device state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  if (subscribe)
  {
    if (playerc_laser_subscribe(laser->proxy, PLAYER_READ_MODE) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  // Construct figures
  laser->scan_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);
  rtk_fig_origin(laser->scan_fig, ox, oy, oa);
    
  // Construct the menu
  snprintf(label, sizeof(label), "Laser %d", index);
  laser->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  laser->subscribe_item = rtk_menuitem_create(laser->menu, "Subscribe", 1);
  laser->res025_item = rtk_menuitem_create(laser->menu, "0.25 deg resolution", 0);
  laser->res050_item = rtk_menuitem_create(laser->menu, "0.50 deg resolution", 0);
  laser->res100_item = rtk_menuitem_create(laser->menu, "1.00 deg resolution", 0);

  // Set the initial menu state
  rtk_menuitem_check(laser->subscribe_item, laser->proxy->info.subscribed);

  return laser;
}


// Destroy a laser device
void laser_destroy(laser_t *laser)
{
  if (laser->proxy->info.subscribed)
    playerc_laser_unsubscribe(laser->proxy);
  playerc_laser_destroy(laser->proxy);

  rtk_fig_destroy(laser->scan_fig);

  rtk_menuitem_destroy(laser->res025_item);
  rtk_menuitem_destroy(laser->res050_item);
  rtk_menuitem_destroy(laser->res100_item);
  rtk_menuitem_destroy(laser->subscribe_item);
  rtk_menu_destroy(laser->menu);
  
  free(laser);
}


// Update a laser device
void laser_update(laser_t *laser)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(laser->subscribe_item))
  {
    if (!laser->proxy->info.subscribed)
      if (playerc_laser_subscribe(laser->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (laser->proxy->info.subscribed)
      if (playerc_laser_unsubscribe(laser->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(laser->subscribe_item, laser->proxy->info.subscribed);

  // See if the resolution has been changed
  if (rtk_menuitem_isactivated(laser->res025_item))
  {
    if (laser->proxy->info.subscribed)
      if (playerc_laser_configure(laser->proxy, -M_PI/2, +M_PI/2, 0.25*M_PI/180, 1) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  if (rtk_menuitem_isactivated(laser->res050_item))
  {
    if (laser->proxy->info.subscribed)
      if (playerc_laser_configure(laser->proxy, -M_PI/2, +M_PI/2, 0.50*M_PI/180, 1) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  if (rtk_menuitem_isactivated(laser->res100_item))
  {
    if (laser->proxy->info.subscribed)
      if (playerc_laser_configure(laser->proxy, -M_PI/2, +M_PI/2, 1.00*M_PI/180, 1) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  if (laser->proxy->info.subscribed)
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




