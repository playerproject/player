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
 * Desc: Blob finder interface
 * Author: Andrew Howard
 * Date: 26 May 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "playerv.h"


// Update the blobfinder configuration
//void blobfinder_update_config(blobfinder_t *blobfinder);

// Draw the blobfinder scan
void blobfinder_draw(blobfinder_t *blobfinder);


// Create a blobfinder device
blobfinder_t *blobfinder_create(mainwnd_t *mainwnd, opt_t *opt,
                        playerc_client_t *client, int index, int subscribe)
{
  char section[64];
  char label[64];
  blobfinder_t *blobfinder;
  
  blobfinder = malloc(sizeof(blobfinder_t));
  blobfinder->datatime = 0;
  blobfinder->proxy = playerc_blobfinder_create(client, index);
  
  // Construct the menu
  snprintf(label, sizeof(label), "blobfinder %d", index);
  blobfinder->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  blobfinder->subscribe_item = rtk_menuitem_create(blobfinder->menu, "Subscribe", 1);
  blobfinder->stats_item = rtk_menuitem_create(blobfinder->menu, "Show stats", 1);

  snprintf(section, sizeof(section), "blobfinder:%d", index);

  // Set the initial menu state
  rtk_menuitem_check(blobfinder->subscribe_item, subscribe);
  rtk_menuitem_check(blobfinder->stats_item, 0);
  
  // Default scale for drawing the image (m/pixel)
  blobfinder->scale = 0.01;
    
  // Construct figures
  blobfinder->image_init = 0;
  blobfinder->image_fig = rtk_fig_create(mainwnd->canvas, NULL, 99);
  rtk_fig_movemask(blobfinder->image_fig, RTK_MOVE_TRANS);

  return blobfinder;
}


// Destroy a blobfinder device
void blobfinder_destroy(blobfinder_t *blobfinder)
{
  // Destroy figures
  rtk_fig_destroy(blobfinder->image_fig);

  // Destroy menu items
  rtk_menuitem_destroy(blobfinder->stats_item);
  rtk_menuitem_destroy(blobfinder->subscribe_item);
  rtk_menu_destroy(blobfinder->menu);

  // Unsubscribe/destroy the proxy
  if (blobfinder->proxy->info.subscribed)
    playerc_blobfinder_unsubscribe(blobfinder->proxy);
  playerc_blobfinder_destroy(blobfinder->proxy);
  
  free(blobfinder);
}


// Update a blobfinder device
void blobfinder_update(blobfinder_t *blobfinder)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(blobfinder->subscribe_item))
  {
    if (!blobfinder->proxy->info.subscribed)
      if (playerc_blobfinder_subscribe(blobfinder->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  else
  {
    if (blobfinder->proxy->info.subscribed)
      if (playerc_blobfinder_unsubscribe(blobfinder->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
  }
  rtk_menuitem_check(blobfinder->subscribe_item, blobfinder->proxy->info.subscribed);

  // Draw in the blobfinder scan if it has been changed.
  if (blobfinder->proxy->info.subscribed)
  {
    if (blobfinder->proxy->info.datatime != blobfinder->datatime)
      blobfinder_draw(blobfinder);
    blobfinder->datatime = blobfinder->proxy->info.datatime;
  }
  else
  {
    rtk_fig_show(blobfinder->image_fig, 0);
    blobfinder->datatime = 0;
  }
}

#define PX(ix) ((ix - blobfinder->proxy->width/2) * blobfinder->scale)
#define PY(iy) ((blobfinder->proxy->height/2 - iy) * blobfinder->scale)
#define DX(ix) ((ix) * blobfinder->scale)
#define DY(iy) ((iy) * blobfinder->scale)

// Draw the blobfinder scan
void blobfinder_draw(blobfinder_t *blobfinder)
{
  int i;
  char text[64];
  double ox, oy, dx, dy;
  playerc_blobfinder_blob_t *blob;
  int sizex, sizey;
  double scalex, scaley;

  rtk_fig_show(blobfinder->image_fig, 1);
  rtk_fig_clear(blobfinder->image_fig);

  // Set the initial pose of the image if it hasnt already been set.
  if (blobfinder->image_init == 0)
  {
    rtk_canvas_get_size(blobfinder->image_fig->canvas, &sizex, &sizey);
    rtk_canvas_get_scale(blobfinder->image_fig->canvas, &scalex, &scaley);
    rtk_fig_origin(blobfinder->image_fig, -sizex * scalex / 4, sizey * scaley / 4, 0);
    blobfinder->image_init = 1;
  }

  // Draw an opaque rectangle on which to render the image.
  rtk_fig_color_rgb32(blobfinder->image_fig, 0xFFFFFF);
  rtk_fig_rectangle(blobfinder->image_fig, 0, 0, 0,
                    DX(blobfinder->proxy->width), DY(blobfinder->proxy->height), 1);
  rtk_fig_color_rgb32(blobfinder->image_fig, 0x000000);
  rtk_fig_rectangle(blobfinder->image_fig, 0, 0, 0,
                    DX(blobfinder->proxy->width), DY(blobfinder->proxy->height), 0);

  // Draw the blobs.
  for (i = 0; i < blobfinder->proxy->blob_count; i++)
  {
    blob = blobfinder->proxy->blobs + i;

    ox = PX((blob->right + blob->left) / 2);
    oy = PY((blob->bottom + blob->top) / 2);
    dx = DX(blob->right - blob->left);
    dy = DY(blob->bottom - blob->top);

    rtk_fig_color_rgb32(blobfinder->image_fig, blob->color);
    rtk_fig_rectangle(blobfinder->image_fig, ox, oy, 0, dx, dy, 0);

    ox = PX(blob->x);
    oy = PY(blob->y);
    rtk_fig_line(blobfinder->image_fig, ox, PY(blob->bottom), ox, PY(blob->top));
    rtk_fig_line(blobfinder->image_fig, PX(blob->left), oy, PX(blob->right), oy);

    // Draw in the stats
    if (rtk_menuitem_ischecked(blobfinder->stats_item))
    {
      ox = PX(blob->x);
      oy = PY(blob->bottom);
      snprintf(text, sizeof(text), "ch %d\narea %d", blob->channel, blob->area);
      rtk_fig_text(blobfinder->image_fig, ox, oy, 0, text);
    }
  }
}




