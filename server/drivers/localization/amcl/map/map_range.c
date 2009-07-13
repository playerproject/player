
/**************************************************************************
 * Desc: Range routines
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "map.h"
#include <libplayerinterface/error.h>

// Extract a single range reading from the map.  Unknown cells and/or
// out-of-bound cells are treated as occupied, which makes it easy to
// use Stage bitmap files.
double map_calc_range(map_t *map, double ox, double oy, double oa, double max_range)
{
  int i, j;
  int ai, aj, bi, bj;
  double dx, dy;
  map_cell_t *cell;
  
  if (fabs(cos(oa)) > fabs(sin(oa)))
  {
    ai = (int) MAP_GXWX(map, ox);
    bi = (int) MAP_GXWX(map, ox + max_range * cos(oa));
    
    aj = (int) MAP_GYWY(map, oy);
    dy = tan(oa) * map->scale;

    if (ai < bi)
    {
      for (i = ai; i < bi; i++)
      {
        j = (int) MAP_GYWY(map, oy + (i - ai) * dy);
        if (MAP_VALID(map, i, j))
        {
          cell = map->cells + MAP_INDEX(map, i, j);
          if (cell->occ_state >= 0)
            return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
        }
        else
            return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;          
      }
    }
    else
    {
      for (i = ai; i > bi; i--)
      {
        j = (int) MAP_GYWY(map, oy + (i - ai) * dy);
        if (MAP_VALID(map, i, j))
        {
          cell = map->cells + MAP_INDEX(map, i, j);
          if (cell->occ_state >= 0)
            return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
        }
        else
          return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
      }
    }
  }
  else
  {
    ai = (int) MAP_GXWX(map, ox);
    dx = tan(M_PI/2 - oa) * map->scale;
    
    aj = (int) MAP_GYWY(map, oy);
    bj = (int) MAP_GYWY(map, oy + max_range * sin(oa));

    if (aj < bj)
    {
      for (j = aj; j < bj; j++)
      {
        i = (int) MAP_GXWX(map, ox + (j - aj) * dx);
        if (MAP_VALID(map, i, j))
        {
          cell = map->cells + MAP_INDEX(map, i, j);
          if (cell->occ_state >= 0)
            return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
        }
        else
          return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
      }
    }
    else
    {
      for (j = aj; j > bj; j--)
      {
        i = (int) MAP_GXWX(map, ox + (j - aj) * dx);
        if (MAP_VALID(map, i, j))
        {
          cell = map->cells + MAP_INDEX(map, i, j);
          if (cell->occ_state >= 0)
            return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
        }
        else
          return sqrt((i - ai) * (i - ai) + (j - aj) * (j - aj)) * map->scale;
      }
    }
  }
  return max_range;
}

