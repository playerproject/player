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
 * Desc: Occupancy grid
 * Author: Andrew Howard
 * Date: 10 Jan 2002
 * CVS: $Id$
**************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "grid.h"


// Create a new grid
grid_t *grid_alloc(int size_x, int size_y, double scale)
{
  grid_t *self;

  self = (grid_t*) malloc(sizeof(grid_t));

  // Make the size odd
  self->size_x = size_x + (1 - size_x % 2);
  self->size_y = size_y + (1 - size_y % 2);
  self->scale = scale;

  // Model parameters
  self->model_occ_inc = 10;
  self->model_emp_inc = -1;
  self->model_occ_thresh = 10;
  self->model_emp_thresh = -1;

  self->max_dist = 0.50;
  
  // Allocate storage for main grid
  self->cells = (grid_cell_t*) calloc(self->size_x * self->size_y, sizeof(grid_cell_t));
    
  // Initialize grid
  grid_reset(self);
  
  return self;
}


// Destroy a grid
void grid_free(grid_t *self)
{
  free(self->cells);
  free(self);
  return;
}


// Reset the grid
void grid_reset(grid_t *self)
{
  int i, j;
  grid_cell_t *cell;

  for (j = 0; j < self->size_y; j++)
  {
    for (i = 0; i < self->size_x; i++)
    {
      cell = self->cells + GRID_INDEX(self, i, j);
      cell->occ_value = 0;
      cell->occ_state = 0;
      cell->occ_dist = self->max_dist;
      cell->visited = 0;
      cell->front = 0;
    }
  }
  return;
}


// Get the cell at the given point
grid_cell_t *grid_get_cell(grid_t *self, double ox, double oy)
{
  int i, j;
  grid_cell_t *cell;

  i = GRID_GXWX(self, ox);
  j = GRID_GYWY(self, oy);
  
  if (!GRID_VALID(self, i, j))
    return NULL;

  cell = self->cells + GRID_INDEX(self, i, j);
  return cell;
}


// See if a point is in free space
int grid_test_free(grid_t *self, double ox, double oy)
{
  grid_cell_t *cell;

  cell = grid_get_cell(self, ox, oy);
  if (cell == NULL)
    return 0;

  if (cell->occ_state == -1)
    return 1;

  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Image manipulation macros
#define GRID_RGBA(r, g, b, a) ((r) | (g << 8) | (b << 16) | (a << 24))


///////////////////////////////////////////////////////////////////////////
// Generate an image (diagnostics)
void grid_draw(grid_t *self)
{
  int i, j;
  grid_cell_t *cell;
  uint32_t *pixel;
  int col;

  if (self->pixels == NULL)
    self->pixels = (uint32_t*) calloc(self->size_x * self->size_y, sizeof(uint32_t));
  
  for (j = 0; j < self->size_y; j++)
  {
    for (i = 0; i < self->size_x; i++)
    {
      cell = self->cells + GRID_INDEX(self, i, j);
      pixel = self->pixels + GRID_INDEX(self, i, j);

      *pixel = GRID_RGBA(0, 0, 0, 0);

      if (cell->occ_value > 0)
      {
        col = 127 - cell->occ_value * 127 / self->model_occ_thresh;
        if (col < 0)
          col = 0;
        *pixel = GRID_RGBA(col, col, col, 128);
      }
      else if (cell->occ_value < 0)
      {
        col = 127 + cell->occ_value * 127 / self->model_emp_thresh;
        if (col > 255)
          col = 255;
        *pixel = GRID_RGBA(col, col, col, 128);
      }
    }
  }

  return;
}
