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
 * Desc: Scanning range-finder (FRF) interface
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "playerv.h"


// Update the SRF configuration
void srf_update_config(srf_t *srf);

// Draw the SRF scan
void srf_draw(srf_t *srf);


// Create a srf device
srf_t *srf_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index)
{
  int subscribe;
  char label[64];
  char section[64];
  srf_t *srf;
  
  srf = malloc(sizeof(srf_t));
  srf->proxy = playerc_srf_create(client, index);
  srf->datatime = 0;

  snprintf(section, sizeof(section), "srf:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "srf %d", index);
  srf->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  srf->subscribe_item = rtk_menuitem_create(srf->menu, "Subscribe", 1);
  srf->res025_item = rtk_menuitem_create(srf->menu, "0.25 deg resolution", 1);
  srf->res050_item = rtk_menuitem_create(srf->menu, "0.50 deg resolution", 1);
  srf->res100_item = rtk_menuitem_create(srf->menu, "1.00 deg resolution", 1);

  // Set the initial menu state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  rtk_menuitem_check(srf->subscribe_item, subscribe);

  // Construct figures
  srf->scan_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);
  
  return srf;
}


// Destroy a srf device
void srf_destroy(srf_t *srf)
{
  if (srf->proxy->info.subscribed)
    playerc_srf_unsubscribe(srf->proxy);
  playerc_srf_destroy(srf->proxy);

  rtk_fig_destroy(srf->scan_fig);

  rtk_menuitem_destroy(srf->res025_item);
  rtk_menuitem_destroy(srf->res050_item);
  rtk_menuitem_destroy(srf->res100_item);
  rtk_menuitem_destroy(srf->subscribe_item);
  rtk_menu_destroy(srf->menu);
  
  free(srf);
}


// Update a srf device
void srf_update(srf_t *srf)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(srf->subscribe_item))
  {
    if (!srf->proxy->info.subscribed)
    {
      if (playerc_srf_subscribe(srf->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);

      // Get the SRF geometry
      if (playerc_srf_get_geom(srf->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);

      rtk_fig_origin(srf->scan_fig,
                     srf->proxy->pose[0],
                     srf->proxy->pose[1],
                     srf->proxy->pose[2]);
    }
  }
  else
  {
    if (srf->proxy->info.subscribed)
      if (playerc_srf_unsubscribe(srf->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(srf->subscribe_item, srf->proxy->info.subscribed);

  // Update the configuration stuff
  if (srf->proxy->info.subscribed)
    srf_update_config(srf);
  
  if (srf->proxy->info.subscribed)
  {
    // Draw in the SRF scan if it has been changed.
    if (srf->proxy->info.datatime != srf->datatime)
    {
      srf_draw(srf);
      srf->datatime = srf->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the SRF.
    rtk_fig_show(srf->scan_fig, 0);
  }
}


// Update the SRF configuration
void srf_update_config(srf_t *srf)
{
  int update;
  double min, max;
  int res, intensity;
  
  update = 0;
  if (rtk_menuitem_isactivated(srf->res025_item))
  {
    res = 25; min = -50*M_PI/180; max = +50*M_PI/180; update = 1;
  }
  if (rtk_menuitem_isactivated(srf->res050_item))
  {
    res = 50; min = -M_PI/2; max = +M_PI/2; update = 1;
  }
  if (rtk_menuitem_isactivated(srf->res100_item))
  {
    res = 100; min = -M_PI/2; max = +M_PI/2; update = 1;
  }

  if (update)
    if (playerc_srf_set_config(srf->proxy, min, max, res, intensity) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);

  if (playerc_srf_get_config(srf->proxy, &min, &max, &res, &intensity) != 0)
    PRINT_ERR1("libplayerc error: %s", playerc_errorstr);

  rtk_menuitem_check(srf->res025_item, (res == 25));
  rtk_menuitem_check(srf->res050_item, (res == 50));
  rtk_menuitem_check(srf->res100_item, (res == 100));
}  


// Draw the SRF scan
void srf_draw(srf_t *srf)
{
  int i;
  double ax, ay, bx, by;

  rtk_fig_show(srf->scan_fig, 1);      
  rtk_fig_clear(srf->scan_fig);
  rtk_fig_color_rgb32(srf->scan_fig, COLOR_SRF_SCAN);

  // Draw in the SRF itself
  rtk_fig_rectangle(srf->scan_fig, 0, 0, 0,
                    srf->proxy->size[0], srf->proxy->size[1], 0);

  // Draw in the range scan
  for (i = 0; i < srf->proxy->scan_count; i++)
  {
    bx = srf->proxy->point[i][0];
    by = srf->proxy->point[i][1];
    if (i == 0)
    {
      ax = bx;
      ay = by;
    }
    rtk_fig_line(srf->scan_fig, ax, ay, bx, by);
    ax = bx;
    ay = by;
  }

  // Draw in the intensity scan
  for (i = 0; i < srf->proxy->scan_count; i++)
  {
    if (srf->proxy->intensity[i] == 0)
      continue;
    ax = srf->proxy->point[i][0];
    ay = srf->proxy->point[i][1];
    rtk_fig_rectangle(srf->scan_fig, ax, ay, 0, 0.05, 0.05, 1);
  }
}




