
/**************************************************************************
 * Desc: Local map
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "inav_map.h"


// Pre-compute the dist LUT
void imap_init_dist_lut(imap_t *imap);

// Distance LUT sorting function
int imap_init_dist_lut_cmp(const void *a, const void *b);


// Create a new imap
imap_t *imap_alloc(int size_x, int size_y, double scale,
                   double max_occ_dist, double max_fit_dist)
{
  imap_t *imap;

  imap = (imap_t*) malloc(sizeof(imap_t));

  // Assume we start at (0, 0)
  imap->origin_x = 0;
  imap->origin_y = 0;
  
  // Make the size odd
  imap->size_x = size_x + (1 - size_x % 2);
  imap->size_y = size_y + (1 - size_y % 2);
  imap->scale = scale;

  // Model parameters
  imap->model_occ_inc = 10;
  imap->model_emp_inc = -1;
  imap->model_occ_max = 20;
  imap->model_emp_min = -2;
  imap->model_occ_thresh = 10;
  imap->model_emp_thresh = -1;

  assert(max_occ_dist >= max_fit_dist);
  imap->max_occ_dist = max_occ_dist;
  imap->max_fit_dist = max_fit_dist;
  
  // Allocate storage for main imap
  imap->cells = (imap_cell_t*) calloc(imap->size_x * imap->size_y, sizeof(imap_cell_t));
  
  // Allocate storage for the image to draw
  imap->image = (uint16_t*) calloc(imap->size_x * imap->size_y, sizeof(uint16_t));

  // Pre-compute LUT's
  imap_init_dist_lut(imap);

  // Initialize imap
  imap_reset(imap);
  
  return imap;
}


// Destroy a imap
void imap_free(imap_t *imap)
{
  free(imap->image);
  free(imap->dist_lut);
  free(imap->cells);
  free(imap);
  return;
}


// Reset the imap
void imap_reset(imap_t *imap)
{
  int i, j;
  imap_cell_t *cell;

  for (j = 0; j < imap->size_y; j++)
  {
    for (i = 0; i < imap->size_x; i++)
    {
      cell = imap->cells + IMAP_INDEX(imap, i, j);

      cell->occ_value = 0;
      cell->occ_state = 0;
      cell->occ_dist = imap->max_occ_dist;
      cell->occ_di = 0;
      cell->occ_dj = 0;
    }
  }
  return;
}


// Translate the map a discrete number of cells in x and/or y
void imap_translate(imap_t *imap, int di, int dj)
{
  int i, j;
  imap_cell_t *dst, *src;

  // Translate x-axis
  if (di > 0)
  {
    for (j = 0; j < imap->size_y; j++)
    {
      dst = imap->cells + IMAP_INDEX(imap, 0, j);
      src = imap->cells + IMAP_INDEX(imap, di, j);
      memmove(dst, src, (imap->size_x - di) * sizeof(imap_cell_t));

      for (i = imap->size_x - di; i < imap->size_x; i++)
      {
        dst = imap->cells + IMAP_INDEX(imap, i, j);
        dst->occ_value = 0;
        dst->occ_state = 0;
        dst->occ_dist = imap->max_occ_dist;
        dst->occ_di = 0;
        dst->occ_dj = 0;
      }
    }
  }
  else if (di < 0)
  {
    for (j = 0; j < imap->size_y; j++)
    {
      dst = imap->cells + IMAP_INDEX(imap, -di, j);
      src = imap->cells + IMAP_INDEX(imap, 0, j);
      memmove(dst, src, (imap->size_x + di) * sizeof(imap_cell_t));

      for (i = 0; i < -di; i++)
      {
        dst = imap->cells + IMAP_INDEX(imap, i, j);
        dst->occ_value = 0;
        dst->occ_state = 0;
        dst->occ_dist = imap->max_occ_dist;
        dst->occ_di = 0;
        dst->occ_dj = 0;
      }
    }
  }
  
  // Translate y-axis
  if (dj > 0)
  {
    for (j = 0; j < imap->size_y - dj; j++)
    {
      dst = imap->cells + IMAP_INDEX(imap, 0, j);
      src = imap->cells + IMAP_INDEX(imap, 0, j + dj);
      memmove(dst, src, imap->size_x * sizeof(imap_cell_t));
    }

    for (; j < imap->size_y; j++)
    {
      for (i = 0; i < imap->size_x; i++)
      {
        dst = imap->cells + IMAP_INDEX(imap, i, j);
        dst->occ_value = 0;
        dst->occ_state = 0;
        dst->occ_dist = imap->max_occ_dist;
        dst->occ_di = 0;
        dst->occ_dj = 0;
      }
    }
  }
  else if (dj < 0)
  {
    for (j = imap->size_y - 1; j >= -dj; j--)
    {
      dst = imap->cells + IMAP_INDEX(imap, 0, j);
      src = imap->cells + IMAP_INDEX(imap, 0, j + dj);
      memmove(dst, src, imap->size_x * sizeof(imap_cell_t));
    }

    for (; j >= 0; j--)
    {
      for (i = 0; i < imap->size_x; i++)
      {
        dst = imap->cells + IMAP_INDEX(imap, i, j);
        dst->occ_value = 0;
        dst->occ_state = 0;
        dst->occ_dist = imap->max_occ_dist;
        dst->occ_di = 0;
        dst->occ_dj = 0;
      }
    }
  }

  // Shift the origin
  imap->origin_x += di * imap->scale;
  imap->origin_y += dj * imap->scale;
  
  return;
}


// Pre-compute the dist LUT
void imap_init_dist_lut(imap_t *imap)
{
  int s, di, dj;
  double dr;
  
  s = (int) ceil(imap->max_occ_dist / imap->scale);

  imap->dist_lut_len = 0;
  imap->dist_lut = (imap_dist_lut_t*) calloc((2 * s + 1) * (2 * s + 1), sizeof(imap_dist_lut_t));
  
  for (dj = -s; dj <= +s; dj++)
  {
    for (di = -s; di <= +s; di++)
    {
      dr = imap->scale * sqrt(di * di + dj * dj);
      if (dr > imap->max_occ_dist)
        continue;
      
      imap->dist_lut[imap->dist_lut_len].di = di;
      imap->dist_lut[imap->dist_lut_len].dj = dj;
      imap->dist_lut[imap->dist_lut_len].dr = dr;
      imap->dist_lut_len++;
    }
  }

  // Sort the lookup table in ascending range order
  qsort(imap->dist_lut, imap->dist_lut_len, sizeof(imap->dist_lut[0]),
        imap_init_dist_lut_cmp);
  
  return;
}


// Distance LUT sorting function
int imap_init_dist_lut_cmp(const void *a, const void *b)
{
  if (((imap_dist_lut_t*) a)->dr < ((imap_dist_lut_t*) b)->dr)
    return -1;
  if (((imap_dist_lut_t*) a)->dr > ((imap_dist_lut_t*) b)->dr)
    return +1;
  return 0;
}


// Get the cell at the given point
imap_cell_t *imap_get_cell(imap_t *imap, double ox, double oy, double oa)
{
  int i, j;
  imap_cell_t *cell;

  i = IMAP_GXWX(imap, ox);
  j = IMAP_GYWY(imap, oy);
  
  if (!IMAP_VALID(imap, i, j))
    return NULL;

  cell = imap->cells + IMAP_INDEX(imap, i, j);
  return cell;
}


