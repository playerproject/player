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
#include <stdlib.h>
#include <string.h>
#if defined WIN32
  #include <replace.h>
#endif
#include "playerv.h"


// Update the map configuration
void map_update_config(map_t *map);

// Draw the map scan
void map_draw(map_t *map);


// Create a map device
map_t *map_create(mainwnd_t *mainwnd, opt_t *opt, playerc_client_t *client,
                      int index, const char *drivername, int subscribe)
{
  char label[64];
  char section[64];
  map_t *map;

  map = malloc(sizeof(map_t));
  map->proxy = playerc_map_create(client, index);
  map->drivername = strdup(drivername);
  map->datatime = 0;

  snprintf(section, sizeof(section), "map:%d", index);

  // Construct the menu
  snprintf(label, sizeof(label), "map:%d (%s)", index, map->drivername);
  map->menu = rtk_menu_create_sub(mainwnd->device_menu, label);
  map->subscribe_item = rtk_menuitem_create(map->menu, "Subscribe", 1);
  map->continuous_item = rtk_menuitem_create(map->menu, "continuous update", 1);
  // Set the initial menu state
  rtk_menuitem_check(map->subscribe_item, subscribe);

  // Construct figures
  map->fig = rtk_fig_create(mainwnd->canvas, NULL, -10);

  return map;
}


// Destroy a map device
void map_destroy(map_t *map)
{
  if (map->proxy->info.subscribed)
    playerc_map_unsubscribe(map->proxy);

  playerc_map_destroy(map->proxy);
  rtk_fig_destroy(map->fig);
  free(map->drivername);
  free(map);

  return;
}


// Update a map device
void map_update(map_t *map)
{
  // Update the device subscription
  if (rtk_menuitem_ischecked(map->subscribe_item))
  {
    if (!map->proxy->info.subscribed)
    {
      if (playerc_map_subscribe(map->proxy, PLAYER_OPEN_MODE) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
      // download a map
      if (playerc_map_get_map( map->proxy ) >= 0)
        map_draw( map );
    }
  }
  else
  {
    if (map->proxy->info.subscribed)
      if (playerc_map_unsubscribe(map->proxy) != 0)
        PRINT_ERR1("libplayerc error: %s", playerc_error_str());
  }
  rtk_menuitem_check(map->subscribe_item, map->proxy->info.subscribed);

  if (map->proxy->info.subscribed)
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
			if (time_diff > MAP_UPDATE_TIME)
			{
				playerc_map_get_map(map->proxy);
				map_draw(map);
				old_time = time;
			}
    }
  }
  else
  {
    // Dont draw the map.
    rtk_fig_show(map->fig, 0);
  }
}

void map_mark_cells(char* drawn,int mapWidth,int mapHeight,int startX,int startY,int endX,int endY)
{
   int i,j;
   for(i=startY;i<=endY;i++)
      for(j=startX;j<=endX;j++)
          drawn[j+i*mapWidth]=TRUE;

}
// Draw the map scan
void map_draw(map_t *map)
{
  double scale = map->proxy->resolution;
  double color;
  char val;
  char* cellsDrawn=(char*)calloc(map->proxy->width*map->proxy->height,sizeof(char));
  int mapWidth=map->proxy->width;
  int mapHeight=map->proxy->height;
  int x,y;
  double ox,oy;
  double rectangleWidth,rectangleHeight;
  int startx, starty;
  int endx,endy;
  rtk_fig_show(map->fig, 1);
  rtk_fig_clear(map->fig);

  puts( "map draw" );
  
  rtk_fig_color(map->fig, 0.5, 0.5, 0.5 ); 
  rtk_fig_rectangle(map->fig, 0,0,0, mapWidth * scale, mapHeight* scale, 1 ); 

  for( y=0; y<mapHeight; y++ )
    for( x=0; x<mapWidth; x++)
    {
      if(cellsDrawn[x+y*mapWidth]==TRUE)
        continue;
        startx=x;
        starty=y;
        endy=mapHeight-1;


        val = map->proxy->cells[ x + y * mapWidth ];
        if(val==0)
          continue;

	while(x < mapWidth)
        {
          int yy  = y;
          while(yy+1 < mapHeight)
          {
            if(map->proxy->cells[x+(yy+1)*mapWidth]==val)
              yy++;
            else
              break;
          }
          if( yy < endy )
            endy=yy;
          if(x+1<mapWidth && 
            map->proxy->cells[(x+1)+y*mapWidth]==val && 
            cellsDrawn[(x+1)+y*mapWidth]==FALSE)
            x++;
          else
            break;
        }
        endx=x;
        map_mark_cells(cellsDrawn,mapWidth,mapHeight,startx,starty,endx,endy);
        rectangleWidth=(endx-startx+1)*scale;
        rectangleHeight=(endy-starty+1)*scale;
        ox=((startx-mapWidth/2.0f)*scale+rectangleWidth/2.0f);
        oy=((starty-mapHeight/2.0f)*scale+rectangleHeight/2.0f);
	

        color = (double)val/map->proxy->data_range; // scale to[-1,1]
        color *= -1.0; //flip sign for coloring occupied to black
        color = (color + 1.0)/2.0; // scale to [0,1]
        rtk_fig_color(map->fig, color, color, color );
        rtk_fig_rectangle(map->fig, ox, oy, 0, rectangleWidth, rectangleHeight,1);
   }
  free(cellsDrawn);
  return;
}




