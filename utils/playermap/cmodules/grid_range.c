/*
 * Map maker
 * Copyright (C) 2003 Andrew Howard 
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
 * Desc: Grid range routines
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#include <math.h>
#include <stdlib.h>
#include "grid.h"

// Add a single range reading to the grid
static void grid_add_range(grid_t *grid, double ox, double oy, double oa, double range);

// Update a cell with a new observation
inline static void grid_update_cell(grid_t *grid, int ci, int cj, double mr, double or);


// Add a range scan to the grid
void grid_add_ranges_slow(grid_t *grid, double ox, double oy, double oa,
                          int range_count, double ranges[][2])
{
  int i, j, k, s;
  int ni, nj;
  double ma, mr, or;
  double start, resol;
  grid_cell_t *cell;
  
  i = GRID_GXWX(grid, ox);
  j = GRID_GYWY(grid, oy);

  // Mark cell as visited
  if (GRID_VALID(grid, i, j))
  {
    cell = grid->cells + GRID_INDEX(grid, i, j);
    cell->visited = 1;
  }
  
  s = ceil(8.0 / grid->scale);
  start = ranges[0][1];
  resol = (ranges[range_count - 1][1] - ranges[0][1]) / (range_count - 1);
  
  for (nj = -s; nj <= +s; nj++)
  {
    for (ni = -s; ni <= +s; ni++)
    {
      // Compute the range to the cell
      mr = sqrt(ni * ni + nj * nj) * grid->scale;

      // Compute the angle to the cell, relative to the laser
      ma = atan2(nj, ni) - oa;
      ma = atan2(sin(ma), cos(ma));

      // Compute the corresponding observed range
      k = floor((ma - start) / resol + 0.5);
      if (k < 0 || k >= range_count)
        continue;
      or = ranges[k][0];

      //printf("%f %f %f\n", ma, ranges[k][1], ma - ranges[k][1]);

      // Update the cell
      grid_update_cell(grid, i + ni, j + nj, mr, or);
    }
  }
  
  return;
}


// Add a range scan to the grid
void grid_add_ranges_fast(grid_t *grid, double ox, double oy, double oa,
                         int range_count, double ranges[][2])
{
  int n;
  double pr, pb;
  int i, j;
  grid_cell_t *cell;
  
  i = GRID_GXWX(grid, ox);
  j = GRID_GYWY(grid, oy);

  // Mark cell as visited
  if (GRID_VALID(grid, i, j))
  {
    cell = grid->cells + GRID_INDEX(grid, i, j);
    cell->visited = 1;
  }

  for (n = 0; n < range_count; n++)
  {
    pr = ranges[n][0];
    pb = ranges[n][1];
    grid_add_range(grid, ox, oy, oa + pb, pr);
  }
  return;
}


// Add a single range reading to the grid
void grid_add_range(grid_t *grid, double ox, double oy, double oa, double range)
{
  int i, j;
  int ai, aj, bi, bj;
  double dx, dy, r, dr;

  dx = tan(M_PI/2 - oa) * grid->scale;
  dy = tan(oa) * grid->scale;

  if (fabs(cos(oa)) > fabs(sin(oa)))
  {
    r = 0;
    dr = fabs(grid->scale / cos(oa));
    ai = GRID_GXWX(grid, ox);
    bi = GRID_GXWX(grid, ox + (range + grid->scale) * cos(oa));
    aj = GRID_GYWY(grid, oy);

    if (ai < bi)
    {
      for (i = ai; i < bi; i++, r += dr)
      {
        j = GRID_GYWY(grid, oy + (i - ai) * dy);
        grid_update_cell(grid, i, j, r, range);
      }
    }
    else
    {
      for (i = ai; i > bi; i--, r += dr)
      {
        j = GRID_GYWY(grid, oy + (i - ai) * dy);
        grid_update_cell(grid, i, j, r, range);
      }
    }
  }
  else
  {
    r = 0;
    dr = fabs(grid->scale / sin(oa));
    ai = GRID_GXWX(grid, ox);
    aj = GRID_GYWY(grid, oy);
    bj = GRID_GYWY(grid, oy + (range + grid->scale) * sin(oa));

    if (aj < bj)
    {
      for (j = aj; j < bj; j++, r += dr)
      {
        i = GRID_GXWX(grid, ox + (j - aj) * dx);
        grid_update_cell(grid, i, j, r, range);
      }
    }
    else
    {
      for (j = aj; j > bj; j--, r += dr)
      {
        i = GRID_GXWX(grid, ox + (j - aj) * dx);
        grid_update_cell(grid, i, j, r, range);
      }
    }
  }
  return;
}


// Update the a cell a new observation
inline void grid_update_cell(grid_t *grid, int ci, int cj, double mr, double or)
{
  double z, d;
  grid_cell_t *cell;
  
  if (!GRID_VALID(grid, ci, cj))
    return;

  cell = grid->cells + GRID_INDEX(grid, ci, cj);

  d = 1.5 * grid->scale;
  z = mr - or;

  if (z < -d)
    cell->occ_value += grid->model_emp_inc;
  else if (z < 0 && or < 8.0)
    cell->occ_value += (z / d) * (grid->model_occ_inc - grid->model_emp_inc) + grid->model_occ_inc;
  else if (z < +d && or < 8.0)
    cell->occ_value += (1 - z / d) * grid->model_occ_inc;

  if (cell->occ_value <= grid->model_emp_thresh)
    cell->occ_state = -1;
  if (cell->occ_value >= grid->model_occ_thresh)
    cell->occ_state = +1;

  return;
}


