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
 * Desc: LBD (laser beacon detector) device interface
 * Author: Andrew Howard
 * Date: 14 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "playerv.h"


// Draw the detected beacons
void lbd_draw(lbd_t *lbd);


// Create a lbd device
lbd_t *lbd_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index)
{
  int subscribe;
  char label[64];
  char section[64];
  lbd_t *lbd;
  
  lbd = malloc(sizeof(lbd_t));
  memset(lbd, 0, sizeof(lbd_t));

  // Create a proxy
  lbd->proxy = playerc_lbd_create(client, index);
  lbd->datatime = 0;

  snprintf(section, sizeof(section), "lbd:%d", index);
    
  // Set initial device state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  if (subscribe)
  {
    if (playerc_lbd_subscribe(lbd->proxy, PLAYER_READ_MODE) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  // Create a figure
  lbd->beacon_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);

  // Construct the menu
  snprintf(label, sizeof(label), "Lbd %d", index);
  lbd->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  lbd->subscribe_item = rtk_menuitem_create(lbd->menu, "Subscribe", 1);
  lbd->bits5_item = rtk_menuitem_create(lbd->menu, "5 bits", 0);
  lbd->bits8_item = rtk_menuitem_create(lbd->menu, "8 bits", 0);

  // Set the initial menu state
  rtk_menuitem_check(lbd->subscribe_item, lbd->proxy->info.subscribed);

  return lbd;
}


// Destroy a lbd device
void lbd_destroy(lbd_t *lbd)
{
  if (lbd->proxy->info.subscribed)
    playerc_lbd_unsubscribe(lbd->proxy);
  playerc_lbd_destroy(lbd->proxy);

  rtk_fig_destroy(lbd->beacon_fig);
  rtk_menuitem_destroy(lbd->bits8_item);
  rtk_menuitem_destroy(lbd->bits5_item);
  rtk_menuitem_destroy(lbd->subscribe_item);
  rtk_menu_destroy(lbd->menu);
  free(lbd);
}


// Update a lbd device
void lbd_update(lbd_t *lbd)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(lbd->subscribe_item))
  {
    if (!lbd->proxy->info.subscribed)
      if (playerc_lbd_subscribe(lbd->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (lbd->proxy->info.subscribed)
      if (playerc_lbd_unsubscribe(lbd->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(lbd->subscribe_item, lbd->proxy->info.subscribed);

  // See if the number of bits has changed
  if (rtk_menuitem_isactivated(lbd->bits5_item))
  {    
    if (lbd->proxy->info.subscribed)
      if (playerc_lbd_set_config(lbd->proxy, 5, 0.050) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  if (rtk_menuitem_isactivated(lbd->bits8_item))
  {
    if (lbd->proxy->info.subscribed)
      if (playerc_lbd_set_config(lbd->proxy, 8, 0.050) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  if (lbd->proxy->info.subscribed)
  {
    // Draw in the beacon data if it has changed.
    if (lbd->proxy->info.datatime != lbd->datatime)
    {
      lbd_draw(lbd);
      lbd->datatime = lbd->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the beacon data
    rtk_fig_show(lbd->beacon_fig, 0);
  }
}


// Draw the lbd scan
void lbd_draw(lbd_t *lbd)
{
  int i;
  double ox, oy, oa;
  double wx, wy;
  char text[64];
  playerc_lbd_beacon_t *beacon;

  rtk_fig_show(lbd->beacon_fig, 1);      
  rtk_fig_clear(lbd->beacon_fig);
  rtk_fig_color_rgb32(lbd->beacon_fig, COLOR_LBD_BEACON);

  rtk_fig_rectangle(lbd->beacon_fig, 0, 0, 0, 0.15, 0.15, 0);
  
  for (i = 0; i < lbd->proxy->beacon_count; i++)
  {
    beacon = lbd->proxy->beacons + i;

    ox = beacon->range * cos(beacon->bearing);
    oy = beacon->range * sin(beacon->bearing);
    oa = beacon->orient;

    // TODO: use the configuration info to determine beacon size
    wx = 0.05;
    wy = 0.40;
    
    rtk_fig_rectangle(lbd->beacon_fig, ox, oy, oa, wx, wy, 0);
    rtk_fig_arrow(lbd->beacon_fig, ox, oy, oa, wy, 0.10);
    snprintf(text, sizeof(text), "  %d", beacon->id);
    rtk_fig_text(lbd->beacon_fig, ox, oy, oa, text);
  }
}




