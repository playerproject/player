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
 * Desc: Fixed range finder (FRF) interface
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "playerv.h"


// Update the FRF configuration
void frf_update_config(frf_t *frf);

// Draw the FRF scan
void frf_draw(frf_t *frf);

// Dont draw the FRF scan
void frf_nodraw(frf_t *frf);


// Create a frf device
frf_t *frf_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index)
{
  int i, subscribe;
  char label[64];
  char section[64];
  frf_t *frf;
  
  frf = malloc(sizeof(frf_t));
  frf->proxy = playerc_frf_create(client, index);
  frf->datatime = 0;

  snprintf(section, sizeof(section), "frf:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "frf %d", index);
  frf->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  frf->subscribe_item = rtk_menuitem_create(frf->menu, "Subscribe", 1);

  // Set the initial menu state
  // Set initial device state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  rtk_menuitem_check(frf->subscribe_item, subscribe);

  // Construct figures
  for (i = 0; i < PLAYERC_FRF_MAX_SAMPLES; i++)
    frf->scan_fig[i] = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);

  return frf;
}


// Destroy a frf device
void frf_destroy(frf_t *frf)
{
  int i;
  
  if (frf->proxy->info.subscribed)
    playerc_frf_unsubscribe(frf->proxy);
  playerc_frf_destroy(frf->proxy);

  for (i = 0; i < PLAYERC_FRF_MAX_SAMPLES; i++)
    rtk_fig_destroy(frf->scan_fig[i]);

  rtk_menuitem_destroy(frf->subscribe_item);
  rtk_menu_destroy(frf->menu);
  
  free(frf);
}


// Update a frf device
void frf_update(frf_t *frf)
{
  int i;
  
  // Update the device subscription
  if (rtk_menuitem_ischecked(frf->subscribe_item))
  {
    if (!frf->proxy->info.subscribed)
    {
      if (playerc_frf_subscribe(frf->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("subscribe failed : %s", playerc_errorstr);

      // Get the FRF geometry
      if (playerc_frf_get_geom(frf->proxy) != 0)
        PRINT_ERR1("get_geom failed : %s", playerc_errorstr);    

      for (i = 0; i < frf->proxy->pose_count; i++)
        rtk_fig_origin(frf->scan_fig[i],
                       frf->proxy->poses[i][0],
                       frf->proxy->poses[i][1],
                       frf->proxy->poses[i][2]);
    }
  }
  else
  {
    if (frf->proxy->info.subscribed)
      if (playerc_frf_unsubscribe(frf->proxy) != 0)
        PRINT_ERR1("unsubscribe failed : %s", playerc_errorstr);
  }
  rtk_menuitem_check(frf->subscribe_item, frf->proxy->info.subscribed);

  if (frf->proxy->info.subscribed)
  {
    // Draw in the FRF scan if it has been changed.
    if (frf->proxy->info.datatime != frf->datatime)
      frf_draw(frf);
    frf->datatime = frf->proxy->info.datatime;
  }
  else
  {
    // Dont draw the FRF.
    frf_nodraw(frf);
  }
}


// Draw the FRF scan
void frf_draw(frf_t *frf)
{
  int i;
  double dr, da;

  for (i = 0; i < frf->proxy->scan_count; i++)
  {
    rtk_fig_show(frf->scan_fig[i], 1);      
    rtk_fig_clear(frf->scan_fig[i]);

    // Draw in the FRF itself
    rtk_fig_color_rgb32(frf->scan_fig[i], COLOR_FRF);
    rtk_fig_rectangle(frf->scan_fig[i], 0, 0, 0, 0.01, 0.05, 0);

    // Draw in the range scan
    rtk_fig_color_rgb32(frf->scan_fig[i], COLOR_FRF_SCAN);
    dr = frf->proxy->scan[i];
    da = 20 * M_PI / 180;
    rtk_fig_line(frf->scan_fig[i], 0, 0, dr, 0);
    rtk_fig_line(frf->scan_fig[i], dr, -dr * da/2, dr, +dr * da/2);
  }
}


// Dont draw the FRF scan
void frf_nodraw(frf_t *frf)
{
  int i;

  for (i = 0; i < frf->proxy->scan_count; i++)
    rtk_fig_show(frf->scan_fig[i], 0);
}


