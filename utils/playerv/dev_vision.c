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
vision_t *vision_create(mainwnd_t *mainwnd, opt_t *opt,
                        playerc_client_t *client, int index)
{
  int subscribe;
  char section[64];
  char label[64];
  vision_t *vision;
  
  vision = malloc(sizeof(vision_t));
  vision->datatime = 0;
  vision->proxy = playerc_vision_create(client, index);
  
  // Construct the menu
  snprintf(label, sizeof(label), "vision %d", index);
  vision->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  vision->subscribe_item = rtk_menuitem_create(vision->menu, "Subscribe", 1);
  vision->stats_item = rtk_menuitem_create(vision->menu, "Show stats", 1);

  snprintf(section, sizeof(section), "vision:%d", index);

  // Set the initial menu state
  subscribe = opt_get_int(opt, section, "", 0);
  subscribe = opt_get_int(opt, section, "subscribe", subscribe);
  rtk_menuitem_check(vision->subscribe_item, subscribe);
  rtk_menuitem_check(vision->stats_item, 0);
  
  // Default scale for drawing the image (m/pixel)
  vision->scale = 0.01;
    
  // Construct figures
  vision->image_init = 0;
  vision->image_fig = rtk_fig_create(mainwnd->canvas, NULL, 99);
  rtk_fig_movemask(vision->image_fig, RTK_MOVE_TRANS);

  return vision;
}


// Destroy a vision device
void vision_destroy(vision_t *vision)
{
  // Destroy figures
  rtk_fig_destroy(vision->image_fig);

  // Destroy menu items
  rtk_menuitem_destroy(vision->stats_item);
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

#define PX(ix) ((ix - vision->proxy->width/2) * vision->scale)
#define PY(iy) ((vision->proxy->height/2 - iy) * vision->scale)
#define DX(ix) ((ix) * vision->scale)
#define DY(iy) ((iy) * vision->scale)

// Draw the vision scan
void vision_draw(vision_t *vision)
{
  int i;
  char text[64];
  double ox, oy, dx, dy;
  playerc_vision_blob_t *blob;
  int sizex, sizey;
  double scalex, scaley;

  rtk_fig_show(vision->image_fig, 1);
  rtk_fig_clear(vision->image_fig);

  // Set the initial pose of the image if it hasnt already been set.
  if (vision->image_init == 0)
  {
    rtk_canvas_get_size(vision->image_fig->canvas, &sizex, &sizey);
    rtk_canvas_get_scale(vision->image_fig->canvas, &scalex, &scaley);
    rtk_fig_origin(vision->image_fig, -sizex * scalex / 4, sizey * scaley / 4, 0);
    vision->image_init = 1;
  }

  // Draw an opaque rectangle on which to render the image.
  rtk_fig_color_rgb32(vision->image_fig, 0xFFFFFF);
  rtk_fig_rectangle(vision->image_fig, 0, 0, 0,
                    DX(vision->proxy->width), DY(vision->proxy->height), 1);
  rtk_fig_color_rgb32(vision->image_fig, 0x000000);
  rtk_fig_rectangle(vision->image_fig, 0, 0, 0,
                    DX(vision->proxy->width), DY(vision->proxy->height), 0);

  // Draw the blobs.
  for (i = 0; i < vision->proxy->blob_count; i++)
  {
    blob = vision->proxy->blobs + i;

    ox = PX((blob->right + blob->left) / 2);
    oy = PY((blob->bottom + blob->top) / 2);
    dx = DX(blob->right - blob->left);
    dy = DY(blob->bottom - blob->top);

    rtk_fig_color_rgb32(vision->image_fig, blob->color);
    rtk_fig_rectangle(vision->image_fig, ox, oy, 0, dx, dy, 0);

    ox = PX(blob->x);
    oy = PY(blob->y);
    rtk_fig_line(vision->image_fig, ox, PY(blob->bottom), ox, PY(blob->top));
    rtk_fig_line(vision->image_fig, PX(blob->left), oy, PX(blob->right), oy);

    // Draw in the stats
    if (rtk_menuitem_ischecked(vision->stats_item))
    {
      ox = PX(blob->x);
      oy = PY(blob->bottom);
      snprintf(text, sizeof(text), "ch %d\narea %d", blob->channel, blob->area);
      rtk_fig_text(vision->image_fig, ox, oy, 0, text);
    }
  }
}




