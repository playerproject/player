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
 * Desc: localization device interface
 * Author: Boyoon Jung
 * Date: 20 Nov 2002
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "playerv.h"


// Reset the pose
void localize_reset_pose(localize_t *localize);

// Draw the map
void localize_draw_map(localize_t *localize);

// Draw the hypotheses
void localize_draw_hypoth(localize_t *localize);

// Compute eigen values and eigen vectors of a 2x2 covariance matrix
static void eigen(double cm[][2], double values[], double vectors[][2]);


// Create a localize device
localize_t *localize_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                            int index, const char *drivername, int subscribe)
{
  char label[64];
  char section[64];
  localize_t *localize;
  
  localize = malloc(sizeof(localize_t));
  localize->proxy = playerc_localize_create(client, index);
  localize->drivername = strdup(drivername);
  localize->datatime = 0;

  snprintf(section, sizeof(section), "localize:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "localize:%d (%s)", index, localize->drivername);
  localize->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  localize->subscribe_item = rtk_menuitem_create(localize->menu, "Subscribe", 1);
  localize->reset_item = rtk_menuitem_create(localize->menu, "Reset", 0);
  localize->showmap_item = rtk_menuitem_create(localize->menu, "Show Map", 1);

  // Set the initial menu state
  rtk_menuitem_check(localize->subscribe_item, subscribe);
  rtk_menuitem_check(localize->showmap_item, 1);

  // Construct figures
  localize->map_fig = rtk_fig_create(mainwnd->canvas, NULL, 90);
  rtk_fig_movemask(localize->map_fig, RTK_MOVE_TRANS);
  localize->hypoth_fig = rtk_fig_create(mainwnd->canvas, localize->map_fig, 95);
  rtk_fig_movemask(localize->hypoth_fig, RTK_MOVE_TRANS);

  // Default magnification for the map (1/8th full size)
  localize->map_mag = 8;

  return localize;
}


// Destroy a localize device
void localize_destroy(localize_t *localize)
{
  // Unsubscribe/destroy the proxy
  if (localize->proxy->info.subscribed)
    playerc_localize_unsubscribe(localize->proxy);
  playerc_localize_destroy(localize->proxy);

  // Destroy figures
  rtk_fig_destroy(localize->map_fig);

  // Destroy menu items
  rtk_menuitem_destroy(localize->subscribe_item);
  rtk_menuitem_destroy(localize->reset_item);
  rtk_menuitem_destroy(localize->showmap_item);
  rtk_menu_destroy(localize->menu);
  
  // Free memory
  free(localize->drivername);
  free(localize->map_image);
  free(localize);

  return;
}


// Update a localize device
void localize_update(localize_t *localize)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(localize->subscribe_item))
  {
    if (!localize->proxy->info.subscribed)
    {
	    if (playerc_localize_subscribe(localize->proxy, PLAYER_READ_MODE) != 0)
        PRINT_ERR1("subscribe failed : %s", playerc_error_str());

      // Load the map
      printf("Loading map (this may take some time)...");
      fflush(stdout);
	    if (playerc_localize_get_map(localize->proxy) != 0)
        PRINT_ERR1("get_map_header failed : %s", playerc_error_str());
      printf("done\n");

      // Draw the map
      localize_draw_map(localize);
    }
  }
  else
  {
    if (localize->proxy->info.subscribed)
    {
	    if (playerc_localize_unsubscribe(localize->proxy) != 0)
        PRINT_ERR1("unsubscribe failed : %s", playerc_error_str());
    }
  }
  rtk_menuitem_check(localize->subscribe_item, localize->proxy->info.subscribed);

  // See if the reset button has been pressed
  if (rtk_menuitem_isactivated(localize->reset_item))
    localize_reset_pose(localize);

  // update the screen
  if (localize->proxy->info.subscribed)
  {
    // Show the figures
    rtk_fig_show(localize->map_fig, rtk_menuitem_ischecked(localize->showmap_item));
    rtk_fig_show(localize->hypoth_fig, 1);

    // Draw in the hypothesis if it has been changed.
    if (localize->proxy->info.datatime != localize->datatime)
	    localize_draw_hypoth(localize);
    localize->datatime = localize->proxy->info.datatime;
  }
  else
  {
    // Hide the figures
    rtk_fig_show(localize->map_fig, 0);
    rtk_fig_show(localize->hypoth_fig, 0);    
    localize->datatime = 0;
  }
}


// Reset the pose
void localize_reset_pose(localize_t *localize)
{
  double pose[3];
  double cov[3][3];

  pose[0] = 0.0;
  pose[1] = 0.0;
  pose[2] = 0.0;

  cov[0][0] = 1e3 * 1e3;
  cov[0][1] = 0.0;
  cov[0][2] = 0.0;

  cov[1][0] = 0.0;
  cov[1][1] = 1e3 * 1e3;
  cov[1][2] = 0.0;

  cov[2][0] = 0.0;
  cov[2][1] = 0.0;
  cov[2][2] = 1e3 * 1e3;  

  if (playerc_localize_set_pose(localize->proxy, pose, cov) != 0)
    PRINT_ERR1("set pose failed : %s", playerc_error_str());
  return;
} 

  
// Draw the map
void localize_draw_map(localize_t *localize)
{
  int i, j, mag, index;
  double scale;
  uint16_t col;
  uint32_t *image;
  int size_x, size_y;
  int ssize_x, ssize_y;

  mag = localize->map_mag;
  scale = localize->proxy->map_scale;

  // Original map dimensions
  size_x = localize->proxy->map_size_x;
  size_y = localize->proxy->map_size_y;

  // Scaled map dimensions
  ssize_x = (int) ceil((double) size_x / mag);
  ssize_y = (int) ceil((double) size_y / mag);

  // Set the initial pose of the map
  rtk_fig_origin(localize->map_fig,
                 -ssize_x * scale / 2 - 1.0,
                 +ssize_y * scale / 2 + 1.0, 0);
  

  // Construct an image representing the map
  image = calloc(ssize_x * ssize_y, sizeof(uint32_t));

  // Generate an averaged image
  for (j = 0; j < size_y; j++)
  {
    for (i = 0; i < size_x; i++)
    {
      switch (localize->proxy->map_cells[i + j * size_x])
      {
        case -1:
          col = 255;
          break;
        case 0:
          col = 192;
          break;
        case +1:
          col = 0;
          break;
      }
      index = i / mag + j / mag * ssize_x;
      assert(index >= 0 && index < ssize_x * ssize_y);
      image[index] += col;
    }
  }

  // Draw the image
  rtk_fig_show(localize->map_fig, 1);
  rtk_fig_clear(localize->map_fig);

  for (j = 0; j < ssize_y; j++)
  {
    for (i = 0; i < ssize_x; i++)
    {
      col = image[i + j * ssize_x] / (mag * mag);
      
      rtk_fig_color(localize->map_fig, col / 255.0, col / 255.0, col / 255.0);
      rtk_fig_rectangle(localize->map_fig,
                        (i - ssize_x / 2 + 0.5) * scale, (j - ssize_y / 2 + 0.5) * scale, 0,
                        scale, scale, 1);
    }
  }
      
  rtk_fig_color(localize->map_fig, 0, 0, 0);
  rtk_fig_rectangle(localize->map_fig, 0, 0, 0, ssize_x * scale, ssize_y * scale, 0);

  free(image);
  
  return;
}


// Draw the hypotheses
void localize_draw_hypoth(localize_t *localize)
{
  int i;
  double mag;
  double ox, oy, oa;
  double sx, sy;
  double cov[2][2], eval[2], evec[2][2];
  playerc_localize_hypoth_t *hypoth;

  mag = localize->map_mag;

  rtk_fig_clear(localize->hypoth_fig);
  rtk_fig_color_rgb32(localize->hypoth_fig, COLOR_LOCALIZE);

  for (i = 0; i < localize->proxy->hypoth_count; i++)
  {
    hypoth = localize->proxy->hypoths + i;

    // Get eigen values/vectors
    cov[0][0] = hypoth->cov[0][0];
    cov[0][1] = hypoth->cov[0][1];
    cov[1][1] = hypoth->cov[1][1];
    eigen(cov, eval, evec);
        
    ox = hypoth->mean[0] / mag;
    oy = hypoth->mean[1] / mag;
    oa = atan2(evec[0][1], evec[0][0]);
    
    sx = 6 * sqrt(eval[0]) / mag;
    sy = 6 * sqrt(eval[1]) / mag;

    rtk_fig_line_ex(localize->hypoth_fig, ox, oy, oa, sx);
    rtk_fig_line_ex(localize->hypoth_fig, ox, oy, oa + M_PI / 2, sy);
    rtk_fig_ellipse(localize->hypoth_fig, ox, oy, oa, sx, sy, 0);
  }
  
  return;
}


// Compute eigen values and eigenvectors of a 2x2 covariance matrix
static void eigen(double cm[][2], double values[], double vectors[][2])
{
    double s = (double) sqrt(cm[0][0]*cm[0][0] - 2*cm[0][0]*cm[1][1] +
                             cm[1][1]*cm[1][1] + 4*cm[0][1]*cm[0][1]);
    values[0] = 0.5 * (cm[0][0] + cm[1][1] + s);
    values[1] = 0.5 * (cm[0][0] + cm[1][1] - s);
    vectors[0][0] = -0.5 * (-cm[0][0] + cm[1][1] - s);
    vectors[0][1] = -0.5 * (-cm[0][0] + cm[1][1] + s);
    vectors[1][0] = vectors[1][1] = cm[0][1];
}
