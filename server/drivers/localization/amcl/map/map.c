/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Andrew Howard
 *     Brian Gerkey    
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


/**************************************************************************
 * Desc: Global map (grid-based)
 * Author: Andrew Howard
 * Date: 6 Feb 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"


// Create a new map
map_t *map_alloc(void)
{
  map_t *map;

  map = (map_t*) malloc(sizeof(map_t));

  // Assume we start at (0, 0)
  map->origin_x = 0;
  map->origin_y = 0;
  
  // Make the size odd
  map->size_x = 0;
  map->size_y = 0;
  map->scale = 0;
  map->max_occ_dist = 0;
  
  // Allocate storage for main map
  map->cells = (map_cell_t*) NULL;
  
  return map;
}


// Destroy a map
void map_free(map_t *map)
{
  free(map->cells);
  free(map);
  return;
}


// Get the cell at the given point
map_cell_t *map_get_cell(map_t *map, double ox, double oy, double oa)
{
  int i, j;
  map_cell_t *cell;

  i = (int) MAP_GXWX(map, ox);
  j = (int) MAP_GYWY(map, oy);
  
  if (!MAP_VALID(map, i, j))
    return NULL;

  cell = map->cells + MAP_INDEX(map, i, j);
  return cell;
}


// Update the cspace distance values
void map_update_cspace(map_t *map, double max_occ_dist)
{
  int i, j;
  int ni, nj;
  int s;
  double d;
  map_cell_t *cell, *ncell;

  map->max_occ_dist = max_occ_dist;
  s = (int) ceil(map->max_occ_dist / map->scale);

  // Reset the distance values
  for (j = 0; j < map->size_y; j++)
  {
    for (i = 0; i < map->size_x; i++)
    {
      cell = map->cells + MAP_INDEX(map, i, j);
      cell->occ_dist = map->max_occ_dist;
    }
  }

  // Find all the occupied cells and update their neighbours
  for (j = 0; j < map->size_y; j++)
  {
    for (i = 0; i < map->size_x; i++)
    {
      cell = map->cells + MAP_INDEX(map, i, j);
      if (cell->occ_state != +1)
        continue;
          
      cell->occ_dist = 0;

      // Update adjacent cells
      for (nj = -s; nj <= +s; nj++)
      {
        for (ni = -s; ni <= +s; ni++)
        {
          if (!MAP_VALID(map, i + ni, j + nj))
            continue;

          ncell = map->cells + MAP_INDEX(map, i + ni, j + nj);
          d = map->scale * sqrt(ni * ni + nj * nj);

          if (d < ncell->occ_dist)
            ncell->occ_dist = d;
        }
      }
    }
  }
  
  return;
}
