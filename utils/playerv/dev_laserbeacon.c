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
 * Desc: LaserBeacon device interface
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
void laserbeacon_draw(laserbeacon_t *laserbeacon);


// Create a laserbeacon device
laserbeacon_t *laserbeacon_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client, int index)
{
  int subscribe;
  char label[64];
  char section[64];
  laserbeacon_t *laserbeacon;
  
  laserbeacon = malloc(sizeof(laserbeacon_t));
  memset(laserbeacon, 0, sizeof(laserbeacon_t));

  // Create a proxy
  laserbeacon->proxy = playerc_laserbeacon_create(client, index);
  laserbeacon->datatime = 0;

  snprintf(section, sizeof(section), "laserbeacon:%d", index);
    
  // Set initial device state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  if (subscribe)
  {
    if (playerc_laserbeacon_subscribe(laserbeacon->proxy, PLAYER_READ_MODE) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  // Create a figure
  laserbeacon->beacon_fig = rtk_fig_create(mainwnd->canvas, mainwnd->robot_fig, 1);

  // Construct the menu
  snprintf(label, sizeof(label), "LaserBeacon %d", index);
  laserbeacon->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  laserbeacon->subscribe_item = rtk_menuitem_create(laserbeacon->menu, "Subscribe", 1);
  laserbeacon->bits5_item = rtk_menuitem_create(laserbeacon->menu, "5 bits", 0);
  laserbeacon->bits8_item = rtk_menuitem_create(laserbeacon->menu, "8 bits", 0);

  // Set the initial menu state
  rtk_menuitem_check(laserbeacon->subscribe_item, laserbeacon->proxy->info.subscribed);

  return laserbeacon;
}


// Destroy a laserbeacon device
void laserbeacon_destroy(laserbeacon_t *laserbeacon)
{
  if (laserbeacon->proxy->info.subscribed)
    playerc_laserbeacon_unsubscribe(laserbeacon->proxy);
  playerc_laserbeacon_destroy(laserbeacon->proxy);

  rtk_fig_destroy(laserbeacon->beacon_fig);
  rtk_menuitem_destroy(laserbeacon->bits8_item);
  rtk_menuitem_destroy(laserbeacon->bits5_item);
  rtk_menuitem_destroy(laserbeacon->subscribe_item);
  rtk_menu_destroy(laserbeacon->menu);
  free(laserbeacon);
}


// Update a laserbeacon device
void laserbeacon_update(laserbeacon_t *laserbeacon)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(laserbeacon->subscribe_item))
  {
    if (!laserbeacon->proxy->info.subscribed)
      if (playerc_laserbeacon_subscribe(laserbeacon->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (laserbeacon->proxy->info.subscribed)
      if (playerc_laserbeacon_unsubscribe(laserbeacon->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(laserbeacon->subscribe_item, laserbeacon->proxy->info.subscribed);

  // See if the number of bits has changed
  if (rtk_menuitem_isactivated(laserbeacon->bits5_item))
  {    
    if (laserbeacon->proxy->info.subscribed)
      if (playerc_laserbeacon_configure(laserbeacon->proxy, 5, 0.050) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  if (rtk_menuitem_isactivated(laserbeacon->bits8_item))
  {
    if (laserbeacon->proxy->info.subscribed)
      if (playerc_laserbeacon_configure(laserbeacon->proxy, 8, 0.050) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }

  if (laserbeacon->proxy->info.subscribed)
  {
    // Draw in the beacon data if it has changed.
    if (laserbeacon->proxy->info.datatime != laserbeacon->datatime)
    {
      laserbeacon_draw(laserbeacon);
      laserbeacon->datatime = laserbeacon->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the beacon data
    rtk_fig_show(laserbeacon->beacon_fig, 0);
  }
}


// Draw the laserbeacon scan
void laserbeacon_draw(laserbeacon_t *laserbeacon)
{
  int i;
  double ox, oy, oa;
  double wx, wy;
  char text[64];
  playerc_laserbeacon_beacon_t *beacon;

  rtk_fig_show(laserbeacon->beacon_fig, 1);      
  rtk_fig_clear(laserbeacon->beacon_fig);
  rtk_fig_color_rgb32(laserbeacon->beacon_fig, COLOR_LASERBEACON_BEACON);

  rtk_fig_rectangle(laserbeacon->beacon_fig, 0, 0, 0, 0.15, 0.15, 0);
  
  for (i = 0; i < laserbeacon->proxy->beacon_count; i++)
  {
    beacon = laserbeacon->proxy->beacons + i;

    ox = beacon->range * cos(beacon->bearing);
    oy = beacon->range * sin(beacon->bearing);
    oa = beacon->orient;

    // TODO: use the configuration info to determine beacon size
    wx = 0.05;
    wy = 0.40;
    
    rtk_fig_rectangle(laserbeacon->beacon_fig, ox, oy, oa, wx, wy, 0);
    rtk_fig_arrow(laserbeacon->beacon_fig, ox, oy, oa, wy, 0.10);
    snprintf(text, sizeof(text), "  %d", beacon->id);
    rtk_fig_text(laserbeacon->beacon_fig, ox, oy, oa, text);
  }
}




