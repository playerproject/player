
/**************************************************************************
 * Desc: Local map GUI functions
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <rtk.h>
#include "map.h"


// Draw the occupancy map
void map_draw_occ(map_t *map, rtk_fig_t *fig)
{
  int i, j;
  int col;
  map_cell_t *cell;

  // Draw occupancy
  for (j = 0; j < map->size_y; j++)
  {
    for (i =  0; i < map->size_x; i++)
    {
      cell = map->cells + MAP_INDEX(map, i, j);
      col = 127 - 127 * cell->occ_state;

      rtk_fig_color(fig, col, col, col);
      rtk_fig_rectangle(fig, MAP_WXGX(map, i), MAP_WYGY(map, j), 0,
                        map->scale, map->scale, 1);
    }
  }
  
  return;
}


// Draw the cspace map
void map_draw_cspace(map_t *map, rtk_fig_t *fig)
{
  int i, j;
  int col;
  map_cell_t *cell;

  // Draw occupancy
  for (j = 0; j < map->size_y; j++)
  {
    for (i =  0; i < map->size_x; i++)
    {
      cell = map->cells + MAP_INDEX(map, i, j);
      col = 255 * cell->occ_dist / map->max_occ_dist;

      rtk_fig_color(fig, col, col, col);
      rtk_fig_rectangle(fig, MAP_WXGX(map, i), MAP_WYGY(map, j), 0,
                        map->scale, map->scale, 1);
    }
  }
  
  return;
}


