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
 * Desc: Vision device interface
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "playerv.h"


// Update the vision configuration
//void vision_update_config(vision_t *vision);

// Draw the vision scan
void vision_draw(vision_t *vision);


// Create a vision device
vision_t *vision_create(mainwnd_t *mainwnd, imagewnd_t *imagewnd, opt_t *opt,
                        playerc_client_t *client, int index)
{
  int subscribe;
  char section[64];
  char label[64];
  vision_t *vision;
  
  vision = malloc(sizeof(vision_t));
  vision->datatime = 0;
  vision->proxy = playerc_vision_create(client, index);

  // Set initial device state
  snprintf(section, sizeof(section), "vision:%d", index);
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  if (subscribe)
  {
    if (playerc_vision_subscribe(vision->proxy, PLAYER_READ_MODE) != 0)
      PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
    
  // Construct the menu
  snprintf(label, sizeof(label), "vision %d", index);
  vision->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  vision->subscribe_item = rtk_menuitem_create(vision->menu, "Subscribe", 1);

  // Set the initial menu state
  rtk_menuitem_check(vision->subscribe_item, vision->proxy->info.subscribed);

  // Construct figures
  vision->image_fig = rtk_fig_create(imagewnd->canvas, NULL, 0);
  
  return vision;
}


// Destroy a vision device
void vision_destroy(vision_t *vision)
{
  // Destroy figures
  rtk_fig_destroy(vision->image_fig);

  // Destroy menu items
  rtk_menuitem_destroy(vision->subscribe_item);
  rtk_menu_destroy(vision->menu);

  // Unsubscribe/destroy the proxy
  if (vision->proxy->info.subscribed)
    playerc_vision_unsubscribe(vision->proxy);
  playerc_vision_destroy(vision->proxy);
  
  free(vision);
}


// Update a vision device
void vision_update(vision_t *vision)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(vision->subscribe_item))
  {
    if (!vision->proxy->info.subscribed)
      if (playerc_vision_subscribe(vision->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (vision->proxy->info.subscribed)
      if (playerc_vision_unsubscribe(vision->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(vision->subscribe_item, vision->proxy->info.subscribed);

  // Draw in the vision scan if it has been changed.
  if (vision->proxy->info.subscribed)
  {
    if (vision->proxy->info.datatime != vision->datatime)
      vision_draw(vision);
    vision->datatime = vision->proxy->info.datatime;
  }
  else
  {
    rtk_fig_show(vision->image_fig, 0);
    vision->datatime = 0;
  }
}


// Draw the vision scan
void vision_draw(vision_t *vision)
{
  int i;
  char text[64];
  double ox, oy, dx, dy;
  playerc_vision_blob_t *blob;

  rtk_fig_show(vision->image_fig, 1);      
  rtk_fig_clear(vision->image_fig);

  rtk_fig_color_rgb32(vision->image_fig, 0xFFFFFF);
  rtk_fig_rectangle(vision->image_fig, vision->proxy->width/2, vision->proxy->height/2,
                    0, vision->proxy->width, vision->proxy->height, 1);
  rtk_fig_color_rgb32(vision->image_fig, 0x000000);
  rtk_fig_rectangle(vision->image_fig, vision->proxy->width/2, vision->proxy->height/2,
                    0, vision->proxy->width, vision->proxy->height, 0);

  for (i = 0; i < vision->proxy->blob_count; i++)
  {
    blob = vision->proxy->blobs + i;

    ox = (blob->right + blob->left) / 2;
    oy = (blob->bottom + blob->top) / 2;
    dx = (blob->right - blob->left);
    dy = (blob->bottom - blob->top);

    rtk_fig_color_rgb32(vision->image_fig, blob->color);
    rtk_fig_rectangle(vision->image_fig, ox, oy, 0, dx, dy, 0);

    ox = blob->x;
    oy = blob->y;

    rtk_fig_line(vision->image_fig, ox, blob->bottom, ox, blob->top);
    rtk_fig_line(vision->image_fig, blob->left, oy, blob->right, oy);

    snprintf(text, sizeof(text), "ch %d\narea %d", blob->channel, blob->area);
    rtk_fig_text(vision->image_fig, ox + 4, oy, 0, text);
  }
}




