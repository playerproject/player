
/**************************************************************************
 * Desc: Local map GUI functions
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef INCLUDE_RTKGUI

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


/* FIX
// Draw the occupancy offsets
void imap_draw_dist(imap_t *imap, rtk_fig_t *fig)
{
  int i, j;
  int di, dj;
  double dr;
  double ax, ay, bx, by;
  imap_cell_t *cell;

  rtk_fig_color(fig, 1, 0, 0);
  
  for (j = 0; j < imap->size_y; j++)
  {
    for (i =  0; i < imap->size_x; i++)
    {
      cell = imap->cells + IMAP_INDEX(imap, i, j);
      dr = cell->occ_dist;
      di = cell->occ_di;
      dj = cell->occ_dj;

      if (dr >= imap->max_occ_dist)
        continue;

      if (di == 0 && dj == 0)
        continue;

      ax = IMAP_WXGX(imap, i);
      ay = IMAP_WYGY(imap, j);

      bx = IMAP_WXGX(imap, i + di);
      by = IMAP_WYGY(imap, j + dj);

      rtk_fig_arrow_ex(fig, ax, ay, bx, by, 0.02);
    }
  }

  return;
}
*/

#endif
