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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.
 *
 */
/***************************************************************************
 * Desc: Map interface
 * Author: Richard Vaughan, based on similar code by Andrew Howard
 * Date: 5 March 2005
 * CVS: $Id$
 ***************************************************************************/

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <libplayerwkb/playerwkb.h>
#include "playerv.h"

// Update the map configuration
void vectormap_update_config(vectormap_t *map);

// Draw the map
void vectormap_draw(vectormap_t *map);

// Create a map device
vectormap_t *vectormap_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                      int index, const char *drivername, int subscribe)
{
  char label[64];
  char section[64];
  vectormap_t *map;

  map = malloc(sizeof(vectormap_t));
  map->proxy = playerc_vectormap_create(client, index);
  map->drivername = strdup(drivername);
  map->datatime = 0;

  snprintf(section, sizeof(section), "vectormap:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "vectormap:%d (%s)", index, map->drivername);
  map->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  map->subscribe_item = rtk_menuitem_create(map->menu, "Subscribe", 1);
  map->continuous_item = rtk_menuitem_create(map->menu, "continuous update", 1);
  // Set the initial menu state
  rtk_menuitem_check(map->subscribe_item, subscribe);

  // Construct figures
  map->fig = rtk_fig_create(mainwnd->canvas, NULL, 1);

  return map;
}


// Destroy a map device
void vectormap_destroy(vectormap_t *map)
{
  if (map->proxy->info.subscribed)
    playerc_vectormap_unsubscribe(map->proxy);

  playerc_vectormap_destroy(map->proxy);
  rtk_fig_destroy(map->fig);
  free(map->drivername);
  free(map);

  return;
}


// Update a map device
void vectormap_update(vectormap_t *map)
{
  unsigned ii;

  // Update the device subscription
  if (rtk_menuitem_ischecked(map->subscribe_item))
  {
    if (!map->proxy->info.subscribed)
    {
      if (playerc_vectormap_subscribe(map->proxy, PLAYER_OPEN_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());

      // get the map info
      playerc_vectormap_get_map_info( map->proxy );

      // download intial map data
      for (ii = 0;  ii < map->proxy->layers_count; ++ii)
      {
        if (playerc_vectormap_get_layer_data( map->proxy, ii ))
        {
          PRINT_ERR1("libplayerc error: %s", playerc_error_str());
          return;
        }
      }
      vectormap_draw( map );
    }
  }
  else
  {
    if (map->proxy->info.subscribed)
      if (playerc_vectormap_unsubscribe(map->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
  }
  rtk_menuitem_check(map->subscribe_item, map->proxy->info.subscribed);

  // Dont draw the map.
  if(map->proxy->info.subscribed)
  {
    if(rtk_menuitem_ischecked(map->continuous_item))
    {
			static struct timeval old_time = {0,0};
			struct timeval time;
			double time_diff = 0.0;
			gettimeofday(&time, NULL);
			// i don't use (map->proxy->info.datatime != map->datatime))
			// because some drivers return strange datatimes
			// and the map may change to often for the current map format
			// in playerv.h you can adjust MAP_UPDATE_TIME (default 1 sec)
			time_diff = (double)(time.tv_sec - old_time.tv_sec) +
				(double)(time.tv_usec - old_time.tv_usec)/1000000;
			if (time_diff > VECTORMAP_UPDATE_TIME)
			{
				// get the map info
    				playerc_vectormap_get_map_info( map->proxy );

    				// download intial map data
    				for (ii = 0;  ii < map->proxy->layers_count; ++ii)
    				{
    				    if (playerc_vectormap_get_layer_data( map->proxy, ii ))
    				    {
        				PRINT_ERR1("libplayerc error: %s", playerc_error_str());
        				return;
    				    }
    				}
    				vectormap_draw( map );
				old_time = time;
			}
    }
    rtk_fig_show(map->fig, 1);
  } else
    rtk_fig_show(map->fig, 0);
}

// Draw the map scan
void vectormap_draw(vectormap_t *map)
{
  unsigned ii, jj;
  uint32_t colour = 0xFF0000;
  double xCenter, yCenter;
  const uint8_t * feature;
  size_t feature_count;

  rtk_fig_show(map->fig, 1);
  rtk_fig_clear(map->fig);

  // draw map data
  for (ii = 0;  ii < map->proxy->layers_count; ++ii)
  {
    // get a different colour for each layer the quick way, will duplicate after 6 layers
    colour = colour >> 4 & colour << 24;
    rtk_fig_color_rgb32(map->fig, colour);

    // render the features
    for (jj = 0; jj < map->proxy->layers_data[ii]->features_count; ++jj)
    {
      feature = playerc_vectormap_get_feature_data( map->proxy, ii, jj );
      feature_count = playerc_vectormap_get_feature_data_count( map->proxy, ii, jj );
      if ((feature) && (feature_count > 0)) player_wkb_process_wkb(map->proxy->wkbprocessor, feature, feature_count, (playerwkbcallback_t)(rtk_fig_line), map->fig);
    }
  }

  // draw map extent
  rtk_fig_color_rgb32( map->fig, 0xFF0000 );
  xCenter = map->proxy->extent.x1 - (map->proxy->extent.x1 - map->proxy->extent.x0)/2;
  yCenter = map->proxy->extent.y1 - (map->proxy->extent.y1 - map->proxy->extent.y0)/2;

  rtk_fig_rectangle(
                    map->fig,
                    xCenter,
                    yCenter,
                    0,
                    map->proxy->extent.x1 - map->proxy->extent.x0,
                    map->proxy->extent.y1 - map->proxy->extent.y0,
                    0
                   );

  return;
}
