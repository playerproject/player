
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

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#include "inav_map.h"


// Add a single range reading to the imap
int imap_add_range(imap_t *imap, double ox, double oy, double oa, double range);

// Update a cell with a new observation
inline void imap_update_cell(imap_t *imap, int ci, int cj, int obs);
inline void imap_update_cell_occ(imap_t *imap, int ci, int cj);
inline void imap_update_cell_not_occ(imap_t *imap, int ci, int cj);
inline void imap_update_cell_dist(imap_t *imap, int ci, int cj);


// Add a range scan to the imap
int imap_add_ranges(imap_t *imap, double robot_pose[3], double laser_pose[3],
                    int range_count, double ranges[][2])
{
  int n;
  int modified;
  double ox, oy, oa;
  double pr, pb;

  modified = 0;
  for (n = 0; n < range_count; n++)
  {
    pr = ranges[n][0];
    pb = ranges[n][1];

    ox = robot_pose[0] + laser_pose[0] * cos(robot_pose[2]) - laser_pose[1] * sin(robot_pose[2]);
    oy = robot_pose[1] + laser_pose[0] * sin(robot_pose[2]) + laser_pose[1] * cos(robot_pose[2]);
    oa = robot_pose[2] + laser_pose[2];
    
    modified |= imap_add_range(imap, ox, oy, oa + pb, pr);
  }
  return modified;
}


// Add a single range reading to the imap
int imap_add_range(imap_t *imap, double ox, double oy, double oa, double range)
{
  int i, j;
  int ai, aj, bi, bj;
  double dx, dy;
  int modified;
  double max_range;

  max_range = 7.8;
  
  modified = 0;
  
  if (fabs(cos(oa)) > fabs(sin(oa)))
  {
    ai = IMAP_GXWX(imap, ox);
    bi = IMAP_GXWX(imap, ox + range * cos(oa));
    aj = IMAP_GYWY(imap, oy);
    dy = tan(oa) * imap->scale;

    // Update empty cells
    if (ai < bi)
    {
      for (i = ai; i < bi; i++)
      {
        j = IMAP_GYWY(imap, oy + (i - ai) * dy);
        imap_update_cell(imap, i, j, -1);
      }
    }
    else
    {
      for (i = ai; i > bi; i--)
      {
        j = IMAP_GYWY(imap, oy + (i - ai) * dy);
        imap_update_cell(imap, i, j, -1);
      }
    }

    // Update occupied cell
    if (range < max_range)
    {
      j = IMAP_GYWY(imap, oy + (i - ai) * dy);
      imap_update_cell(imap, i, j, +1);
    }
  }
  else
  {
    ai = IMAP_GXWX(imap, ox);
    dx = tan(M_PI/2 - oa) * imap->scale;
    
    aj = IMAP_GYWY(imap, oy);
    bj = IMAP_GYWY(imap, oy + range * sin(oa));

    // Update empty cells
    if (aj < bj)
    {
      for (j = aj; j < bj; j++)
      {
        i = IMAP_GXWX(imap, ox + (j - aj) * dx);
        imap_update_cell(imap, i, j, -1);
      }
    }
    else
    {
      for (j = aj; j > bj; j--)
      {
        i = IMAP_GXWX(imap, ox + (j - aj) * dx);
        imap_update_cell(imap, i, j, -1);
      }
    }

    // Update occupied cell
    if (range < max_range)
    {
      i = IMAP_GXWX(imap, ox + (j - aj) * dx);
      imap_update_cell(imap, i, j, +1);
    }
  }
  return modified;
}


// Update the a cell a new observation
inline void imap_update_cell(imap_t *imap, int ci, int cj, int obs)
{
  int occ_state;
  imap_cell_t *cell;
  
  if (!IMAP_VALID(imap, ci, cj))
    return;

  cell = imap->cells + IMAP_INDEX(imap, ci, cj);

  if (obs > 0)
  {
    cell->occ_value += imap->model_occ_inc;
    cell->occ_value = MIN(cell->occ_value, imap->model_occ_max);
  }
  else
  {
    cell->occ_value += imap->model_emp_inc;
    cell->occ_value = MAX(cell->occ_value, imap->model_emp_min);
  }
  
  if (cell->occ_value >= imap->model_occ_thresh)
    occ_state = +1;
  else if (cell->occ_value <= imap->model_emp_thresh)
    occ_state = -1;
  else
    occ_state = 0;
  
  if (occ_state == +1 && cell->occ_state != +1)
  {
    cell->occ_state = occ_state;
    imap_update_cell_occ(imap, ci, cj);
  }
  else if (occ_state != +1 && cell->occ_state == +1)
  {
    cell->occ_state = occ_state;
    imap_update_cell_not_occ(imap, ci, cj);
  }
  else
    cell->occ_state = occ_state;    

  return;
}


// Update a cell that has just become occupied.
void imap_update_cell_occ(imap_t *imap, int ci, int cj)
{
  int i, j, k;
  int di, dj;
  double dr;
  imap_cell_t *cell;
  
  cell = imap->cells + IMAP_INDEX(imap, ci, cj);
  
  // Update distances
  for (k = 0; k < imap->dist_lut_len; k++)
  {
    di = imap->dist_lut[k].di;
    dj = imap->dist_lut[k].dj;
    dr = imap->dist_lut[k].dr;
    
    i = ci + di;
    j = cj + dj;

    if (!IMAP_VALID(imap, i, j))
      continue;

    cell = imap->cells + IMAP_INDEX(imap, i, j);
    if (dr < cell->occ_dist)
    {
      cell->occ_dist = dr;
      cell->occ_di = -di;
      cell->occ_dj = -dj;
    }
  }

  return;
}


// Update a cell that was occupied but is no longer occupied.
void imap_update_cell_not_occ(imap_t *imap, int ci, int cj)
{
  int i, j, k;
  int ni, nj;
  imap_cell_t *cell;

  cell = imap->cells + IMAP_INDEX(imap, ci, cj);
  
  // Update ourself, since we are no longer occupied
  imap_update_cell_dist(imap, ci, cj);
  
  // Look through nearby cells and see if there are any that need
  // updating
  for (k = 0; k < imap->dist_lut_len; k++)
  {
    i = ci + imap->dist_lut[k].di;
    j = cj + imap->dist_lut[k].dj;
    if (!IMAP_VALID(imap, i, j))
      continue;

    cell = imap->cells + IMAP_INDEX(imap, i, j);

    // Compute grid coord of nearest occupied cell
    ni = i + cell->occ_di;
    nj = j + cell->occ_dj;

    // If it is the cell that has just changed state,
    // we need to recompute
    if (ni == ci && nj == cj)
      imap_update_cell_dist(imap, i, j);
  }

  return;
}


// Recompute the occupancy distance for a cell
void imap_update_cell_dist(imap_t *imap, int ci, int cj)
{
  int i, j, k;
  int di, dj;
  double dr;
  imap_cell_t *cell, *ncell;

  cell = imap->cells + IMAP_INDEX(imap, ci, cj);
  cell->occ_dist = imap->max_occ_dist;
  cell->occ_di = 0;
  cell->occ_dj = 0;
    
  for (k = 0; k < imap->dist_lut_len; k++)
  {
    di = imap->dist_lut[k].di;
    dj = imap->dist_lut[k].dj;
    dr = imap->dist_lut[k].dr;

    i = ci + di;
    j = cj + dj;
    if (!IMAP_VALID(imap, i, j))
      continue;

    ncell = imap->cells + IMAP_INDEX(imap, i, j);

    if (ncell->occ_state == +1)
    {
      cell->occ_dist = dr;
      cell->occ_di = di;
      cell->occ_dj = dj;
      break;
    }
  }
  return;
}


// Return the distance to the nearest occupied cell.
double imap_occ_dist(imap_t *imap, double ox, double oy)
{
  int i, j;
  imap_cell_t *cell;

  i = IMAP_GXWX(imap, ox);
  j = IMAP_GYWY(imap, oy);

  if (!IMAP_VALID(imap, i, j))
    return imap->max_occ_dist;
  
  cell = imap->cells + IMAP_INDEX(imap, i, j);
  return cell->occ_dist;
}


// Get a vector that points to the nearest occupied cell.
double imap_occ_vector(imap_t *imap, double ox, double oy, double *dx, double *dy)
{
  int i, j;
  imap_cell_t *cell;

  i = IMAP_GXWX(imap, ox);
  j = IMAP_GYWY(imap, oy);

  if (!IMAP_VALID(imap, i, j))
    return imap->max_occ_dist;
  
  cell = imap->cells + IMAP_INDEX(imap, i, j);

  *dx = cell->occ_di * imap->scale;
  *dy = cell->occ_dj * imap->scale;  
  
  return cell->occ_dist;
}
