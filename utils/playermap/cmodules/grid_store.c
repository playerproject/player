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
 * Desc: Storage routines
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "grid.h"


///////////////////////////////////////////////////////////////////////////
// Save the occupancy grid to an image
int grid_save_occ(grid_t *grid, const char *filename)
{
  int i, j;
  int col;
  FILE *file;
  grid_cell_t *cell;

  file = fopen(filename, "w+");
  if (file == NULL)
    return -1;

  // Write pgm header
  fprintf(file, "P5\n%d %d\n%d\n", grid->size_x, grid->size_y, 255);
  
  // Write data here
  for (j = grid->size_y - 1; j >= 0; j--)
  {
    for (i = 0; i < grid->size_x; i++)
    {
      cell = grid->cells + GRID_INDEX(grid, i, j);

      if (cell->occ_value >= 0)
        col = 127 - cell->occ_value * 127 / grid->model_occ_thresh;
      else
        col = 127 + cell->occ_value * 127 / grid->model_emp_thresh;

      if (col < 0)
        col = 0;
      if (col > 255)
        col = 255;

      fputc(col, file);
    }
  }

  fclose(file);

  return 0;
}


///////////////////////////////////////////////////////////////////////////
// Save the robot path to an image
int grid_save_path(grid_t *grid, const char *filename)
{
  int i, j;
  int col;
  FILE *file;
  grid_cell_t *cell;

  file = fopen(filename, "w+");
  if (file == NULL)
    return -1;

  // Write pgm header
  fprintf(file, "P5\n%d %d\n%d\n", grid->size_x, grid->size_y, 255);
  
  // Write data here
  for (j = grid->size_y - 1; j >= 0; j--)
  {
    for (i = 0; i < grid->size_x; i++)
    {
      cell = grid->cells + GRID_INDEX(grid, i, j);

      if (cell->visited)
        col = 0;
      else
        col = 255;

      fputc(col, file);
    }
  }

  fclose(file);

  return 0;
}

