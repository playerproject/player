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
 * Desc: All-purpose grid
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
 **************************************************************************/

#ifndef GRID_H
#define GRID_H

#include <stdint.h>

// Forward declarations
struct _rtk_fig_t;


// Description for a grid single cell
typedef struct _grid_cell_t
{
  // Occupancy value
  int occ_value;
  
  // Occupancy state (-1 = free, 0 = unknown, +1 = occ)
  int occ_state;

  // Distance to nearest occupied cell
  double occ_dist;

  // Non-zero if this cell has been visited by the robot
  int visited;

  // Non-zero if this cell is a frontier
  int front;
    
} grid_cell_t;


// Description for a grid
typedef struct
{
  // Grid dimensions (number of cells)
  int size_x, size_y;

  // Grid scale (m/cell)
  double scale;

  // Model parameters
  int model_occ_inc, model_emp_inc;
  int model_occ_thresh, model_emp_thresh;

  // Maximum cspace distance
  double max_dist;
  
  // The grid data
  grid_cell_t *cells;

  // The grid image (diagnostics)
  uint32_t *pixels;

} grid_t;



/**************************************************************************
 * Basic grid functions
 **************************************************************************/

// Create a new grid
grid_t *grid_alloc(int size_x, int size_y, double scale);

// Destroy a grid
void grid_free(grid_t *self);

// Reset the grid
void grid_reset(grid_t *self);

// Save the occupancy values to an image file
int grid_save_occ(grid_t *self, const char *filename);

// Get the cell at the given point
grid_cell_t *grid_get_cell(grid_t *self, double ox, double oy);

// See if a point is in free space
int grid_test_free(grid_t *self, double ox, double oy);


/**************************************************************************
 * Range functions
 **************************************************************************/

// Add a range scan to the grid (fast-but-sparse)
void grid_add_ranges_fast(grid_t *self, double ox, double oy, double oa,
                          int range_count, double ranges[][2]);

// Add a range scan to the grid (slow-but-dense)
void grid_add_ranges_slow(grid_t *self, double ox, double oy, double oa,
                          int range_count, double ranges[][2]);


/**************************************************************************
 * Diagnostics
 **************************************************************************/

// Generate an image (diagnostics)
void grid_draw(grid_t *self);


/**************************************************************************
 * Grid manipulation macros
 **************************************************************************/


// Convert from grid index to world coords
#define GRID_WXGX(grid, i) (((i) - grid->size_x / 2) * grid->scale)
#define GRID_WYGY(grid, j) (((j) - grid->size_y / 2) * grid->scale)

// Convert from world coords to grid coords
#define GRID_GXWX(grid, x) (floor((x) / grid->scale + 0.5) + grid->size_x / 2)
#define GRID_GYWY(grid, y) (floor((y) / grid->scale + 0.5) + grid->size_y / 2)

// Test to see if the given grid coords lie within the absolute grid bounds.
#define GRID_VALID(grid, i, j) ((i >= 0) && (i < grid->size_x) && (j >= 0) && (j < grid->size_y))

// Compute the cell index for the given grid coords.
#define GRID_INDEX(grid, i, j) ((i) + (j) * grid->size_x)

#endif
