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

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "playerv.h"

// Draw the ranger scan
void ranger_draw(ranger_t *ranger);

// Create a ranger device
ranger_t* ranger_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                        int index, const char *drivername, int subscribe)
{
  char label[64];
  char section[64];
  ranger_t *ranger;

  ranger = malloc(sizeof(ranger_t));

  ranger->proxy = playerc_ranger_create(client, index);
  ranger->drivername = strdup(drivername);
  ranger->datatime = 0;

  snprintf(section, sizeof(section), "ranger:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "ranger:%d (%s)", index, ranger->drivername);
  ranger->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  ranger->subscribe_item = rtk_menuitem_create(ranger->menu, "Subscribe", 1);
  ranger->style_item = rtk_menuitem_create(ranger->menu, "Filled", 1);
  ranger->intns_item = rtk_menuitem_create(ranger->menu, "Draw intensity data", 1);

  // Set the initial menu state
  rtk_menuitem_check(ranger->subscribe_item, subscribe);
  rtk_menuitem_check(ranger->style_item, 1);
  rtk_menuitem_check(ranger->intns_item, 1);

  ranger->mainwnd = mainwnd;
  ranger->scan_fig = NULL;

  return ranger;
}


void ranger_delete_figures(ranger_t *ranger)
{
  int ii;

  if (ranger->scan_fig != NULL)
  {
    for (ii = 0; ii < ranger->proxy->element_count; ii++)
      rtk_fig_destroy(ranger->scan_fig[ii]);
    free(ranger->scan_fig);
    ranger->scan_fig = NULL;
  }
}


// Destroy a ranger device
void ranger_destroy(ranger_t *ranger)
{
  ranger_delete_figures(ranger);

  if (ranger->proxy->info.subscribed)
    playerc_ranger_unsubscribe(ranger->proxy);
  playerc_ranger_destroy(ranger->proxy);

  rtk_menuitem_destroy(ranger->subscribe_item);
  rtk_menuitem_destroy(ranger->style_item);
  rtk_menuitem_destroy(ranger->intns_item);
  rtk_menu_destroy(ranger->menu);

  free(ranger->drivername);

  free(ranger);
}


// Update a ranger device
void ranger_update(ranger_t *ranger)
{
  int ii;

  // Update the device subscription
  if (rtk_menuitem_ischecked(ranger->subscribe_item))
  {
    if (!ranger->proxy->info.subscribed)
    {
      if (playerc_ranger_subscribe(ranger->proxy, PLAYER_OPEN_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());

      // Get the ranger geometry
      if (playerc_ranger_get_geom(ranger->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());

      // Request the device config for min angle and resolution
      if (playerc_ranger_get_config(ranger->proxy, NULL, NULL, NULL, NULL, NULL, NULL, NULL) != 0)
      {
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
        ranger->start_angle = 0.0f;
        ranger->angular_res = 0.0f;
      }
      else
      {
        ranger->start_angle = ranger->proxy->min_angle;
        ranger->angular_res = ranger->proxy->angular_res;
      }

      // Delete any current figures
      ranger_delete_figures(ranger);
      // Create the figures
      if (ranger->proxy->element_count == 1)
      {
        if ((ranger->scan_fig = malloc(sizeof(rtk_fig_t*))) == NULL )
        {
          PRINT_ERR("Failed to allocate memory for a figure to display ranger");
          return;
        }
        ranger->scan_fig[0] = rtk_fig_create(ranger->mainwnd->canvas, ranger->mainwnd->robot_fig, 1);
        rtk_fig_origin(ranger->scan_fig[0],
                       ranger->proxy->device_pose.px,
                       ranger->proxy->device_pose.py,
                       ranger->proxy->device_pose.pyaw);
      }
      else
      {
        if ((ranger->scan_fig = malloc(ranger->proxy->element_count * sizeof(rtk_fig_t*))) == NULL )
        {
          PRINT_ERR1("Failed to allocate memory for %d figures to display ranger", ranger->proxy->element_count);
          return;
        }
        for (ii = 0; ii < ranger->proxy->element_count; ii++)
        {
          ranger->scan_fig[ii] = rtk_fig_create(ranger->mainwnd->canvas, ranger->mainwnd->robot_fig, 1);
          rtk_fig_origin(ranger->scan_fig[ii],
                         ranger->proxy->element_poses[ii].px,
                         ranger->proxy->element_poses[ii].py,
                         ranger->proxy->element_poses[ii].pyaw);
        }
      }
    }
  }
  else
  {
    if (ranger->proxy->info.subscribed)
      if (playerc_ranger_unsubscribe(ranger->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
    ranger_delete_figures(ranger);
  }
  rtk_menuitem_check(ranger->subscribe_item, ranger->proxy->info.subscribed);

  if (ranger->proxy->info.subscribed)
  {
    // Draw in the ranger scan if it has been changed
    if (ranger->proxy->info.datatime != ranger->datatime)
    {
      ranger_draw(ranger);
      ranger->datatime = ranger->proxy->info.datatime;
    }
  }
  else
  {
    // Dont draw the scan
    // This causes a segfault, because the ranger figures were already
    // deleted above.  I don't know whether commenting it out is the right
    // thing to do - BPG.
    /*
    for (ii = 0; ii < ranger->proxy->element_count; ii++)
      rtk_fig_show(ranger->scan_fig[ii], 0);
      */
  }
}

// Draw the ranger scan
void ranger_draw(ranger_t *ranger)
{
  int ii = 0;
  double point1[2], point2[2]; 
  double (*points)[2];
  double temp = 0.0;
  double b, range;

  // Drawing type depends on the assumed sensor type
  // Singular sensors (e.g. sonar sensors):
  //   Draw a cone for each range scan
  // Non-singular sensors (e.g. laser scanner):
  //   Draw the edge of the scan and empty space

  if (ranger->proxy->element_count > 1)
  {
    // Draw sonar-like
    points = calloc(3, sizeof(double)*2);
    temp = 20.0f * M_PI / 180.0f / 2.0f;
    for (ii = 0; ii < ranger->proxy->ranges_count; ii++)
    {
      rtk_fig_show(ranger->scan_fig[ii], 1);
      rtk_fig_clear(ranger->scan_fig[ii]);
      rtk_fig_color_rgb32(ranger->scan_fig[ii], COLOR_SONAR_SCAN);

      // Draw a cone for each range
      // Assume the range is straight ahead (ignore min_angle and angular_res properties)
      points[0][0] = 0.0f;
      points[0][1] = 0.0f;
      range = ranger->proxy->ranges[ii];
      if (range < ranger->proxy->min_range)
        range = ranger->proxy->max_range;
      points[1][0] = range * cos(-temp);
      points[1][1] = range * sin(-temp);
      points[2][0] = range * cos(temp);
      points[2][1] = range * sin(temp);
      rtk_fig_polygon(ranger->scan_fig[ii], 0, 0, 0, 3, points, 1);

      // Draw the sensor itself
      rtk_fig_color_rgb32(ranger->scan_fig[ii], COLOR_LASER);
      rtk_fig_rectangle(ranger->scan_fig[ii], 0, 0, 0,
                        ranger->proxy->element_sizes[ii].sw,
                        ranger->proxy->element_sizes[ii].sl, 0);
    }
    free(points);
    points=NULL;
  }
  else
  {
    // Draw laser-like
    if (rtk_menuitem_ischecked(ranger->style_item))
    {
      // Draw each sensor in turn
      points = calloc(ranger->proxy->ranges_count + 1, sizeof(double)*2);
      rtk_fig_show(ranger->scan_fig[0], 1);
      rtk_fig_clear(ranger->scan_fig[0]);

      // Draw empty space
      points[0][0] = 0.0;
      points[0][1] = 0.0;
      b = ranger->proxy->min_angle;
      for (ii = 0; ii < ranger->proxy->ranges_count; ii++)
      {
        range = ranger->proxy->ranges[ii];
        if (range < ranger->proxy->min_range) {
          points[ii + 1][0] = ranger->proxy->max_range * cos(b);
          points[ii + 1][1] = ranger->proxy->max_range * sin(b);
        }
        else {
          points[ii + 1][0] = ranger->proxy->points[ii].px;
          points[ii + 1][1] = ranger->proxy->points[ii].py;
        }
        b += ranger->proxy->angular_res;
      }
      rtk_fig_color_rgb32(ranger->scan_fig[0], COLOR_LASER_EMP);
      rtk_fig_polygon(ranger->scan_fig[0], 0, 0, 0,
                      ranger->proxy->ranges_count + 1, points, 1);

      // Draw occupied space
      rtk_fig_color_rgb32(ranger->scan_fig[0], COLOR_LASER_OCC);
      for (ii = 1; ii < ranger->proxy->ranges_count; ii++)
      {
        point1[0] = points[ii][0];
        point1[1] = points[ii][1];
        point2[0] = points[ii+1][0];
        point2[1] = points[ii+1][1];
        rtk_fig_line(ranger->scan_fig[0], point1[0], point1[1], point2[0], point2[1]);
      }
      free(points);
      points = NULL;

    }
    else
    {
      rtk_fig_show(ranger->scan_fig[0], 1);
      rtk_fig_clear(ranger->scan_fig[0]);

      rtk_fig_color_rgb32(ranger->scan_fig[0], COLOR_LASER_OCC);
      // Get the first point

      b = ranger->proxy->min_angle;

      range = ranger->proxy->ranges[0];
      if (range < ranger->proxy->min_range) {
        point1[0] = ranger->proxy->max_range * cos(b);
        point1[1] = ranger->proxy->max_range * sin(b);
      }
      else {
        point1[0] = ranger->proxy->points[0].px;
        point1[1] = ranger->proxy->points[0].py;
      }
      // Loop over the rest of the ranges
      for (ii = 1; ii < ranger->proxy->ranges_count; ii++)
      {
        b += ranger->proxy->angular_res;
        range = ranger->proxy->ranges[ii];
        if (range < ranger->proxy->min_range) {
          point2[0] = ranger->proxy->max_range * cos(b);
          point2[1] = ranger->proxy->max_range * sin(b);
        }
        else {
          point2[0] = ranger->proxy->points[ii].px;
          point2[1] = ranger->proxy->points[ii].py;
        }
        // Draw a line from point 1 (previous point) to point 2 (current point)
        rtk_fig_line(ranger->scan_fig[0], point1[0], point1[1], point2[0], point2[1]);
        point1[0] = point2[0];
        point1[1] = point2[1];
      }
    }

    if (rtk_menuitem_ischecked(ranger->intns_item))
    {
      // Draw an intensity scan
      b = ranger->proxy->min_angle;
      if (ranger->proxy->intensities_count > 0)
      {
        for (ii = 0; ii < ranger->proxy->intensities_count; ii++)
        {
          if (ranger->proxy->intensities[ii] != 0)
          {
            range = ranger->proxy->ranges[ii];
            if (range < ranger->proxy->min_range) {
              point1[0] = ranger->proxy->max_range * cos(b);
              point1[1] = ranger->proxy->max_range * sin(b);
            }
            else {
              point1[0] = ranger->proxy->points[ii].px;
              point1[1] = ranger->proxy->points[ii].py;
            }
            rtk_fig_rectangle(ranger->scan_fig[0], point1[0], point1[1], 0, 0.05, 0.05, 1);
          }
          b += ranger->proxy->angular_res;
        }
      }
    }

    // Draw the sensor itself
    rtk_fig_color_rgb32(ranger->scan_fig[0], COLOR_LASER);
    rtk_fig_rectangle(ranger->scan_fig[0], 0, 0, 0, ranger->proxy->device_size.sw, ranger->proxy->device_size.sl, 0);
  }
}
