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
 * Desc: Range scan storage and manipulation
 * Author: Andrew Howard
 * Date: 4 Jul 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "gpc.h"
#include "scan.h"


////////////////////////////////////////////////////////////////////////////
// Create a new solid
scan_solid_t *scan_solid_alloc()
{
  scan_solid_t *self;

  self = calloc(1, sizeof(scan_solid_t));

  self->contour_count = 0;
  self->contour_max_count = 0;
  self->contours = NULL;
  
  return self;
}


////////////////////////////////////////////////////////////////////////////
// Destroy a solid
void scan_solid_free(scan_solid_t *self)
{
  int i;
  
  for (i = 0; i < self->contour_count; i++)
    scan_contour_free(self->contours[i]);
  
  free(self->contours);
  free(self);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Reset a solid
void scan_solid_reset(scan_solid_t *self)
{
  int i;
  
  for (i = 0; i < self->contour_count; i++)
    scan_contour_free(self->contours[i]);
  self->contour_count = 0;

  return;
}


////////////////////////////////////////////////////////////////////////////
// Add a contour to a solid
void scan_solid_append(scan_solid_t *self, scan_contour_t *contour)
{
  // Resize array as necessary
  if (self->contour_count >= self->contour_max_count)
  {
    self->contour_max_count += 100;
    self->contours = realloc(self->contours,
                             self->contour_max_count * sizeof(self->contours[0]));
  }

  assert(self->contour_count < self->contour_max_count);
  self->contours[self->contour_count++] = contour;

  return;
}


////////////////////////////////////////////////////////////////////////////
// Take union of solid with a contour
void scan_solid_union(scan_solid_t *self, vector_t pose, scan_contour_t *contour)
{
  int i, j;
  vector_t npose;
  scan_point_t *point;
  scan_contour_t *ncontour;
  gpc_polygon apoly, bpoly, cpoly;
  
  // Construct polygon for current points
  apoly.num_contours = self->contour_count;
  apoly.contour = calloc(apoly.num_contours, sizeof(apoly.contour[0]));
  apoly.hole = calloc(apoly.num_contours, sizeof(apoly.hole[0]));

  for (j = 0; j < self->contour_count; j++)
  {
    ncontour = self->contours[j];

    apoly.hole[j] = ncontour->inside;
    apoly.contour[j].num_vertices = ncontour->point_count;
    apoly.contour[j].vertex = calloc(ncontour->point_count, sizeof(apoly.contour[j].vertex[0]));

    for (i = 0; i < ncontour->point_count; i++)
    {
      point = ncontour->points + i;
      apoly.contour[j].vertex[i].x = point->x;
      apoly.contour[j].vertex[i].y = point->y;
    }
  }

  // Construct polygon new contour
  bpoly.num_contours = 1;
  bpoly.contour = calloc(bpoly.num_contours, sizeof(bpoly.contour[0]));
  bpoly.hole = calloc(bpoly.num_contours, sizeof(bpoly.hole[0]));

  bpoly.contour[0].num_vertices = contour->point_count;
  bpoly.contour[0].vertex = calloc(contour->point_count, sizeof(bpoly.contour[0].vertex[0]));

  for (i = 0; i < contour->point_count; i++)
  {
    point = contour->points + i;
    npose = vector_coord_add(vector_set(point->x, point->y, 0), pose);
    bpoly.contour[0].vertex[i].x = npose.v[0];
    bpoly.contour[0].vertex[i].y = npose.v[1];
  }

  // Find the union
  memset(&cpoly, 0, sizeof(cpoly));
  gpc_polygon_clip(GPC_UNION, &apoly, &bpoly, &cpoly);

  // Delete existing contours
  for (i = 0; i < self->contour_count; i++)
    scan_contour_free(self->contours[i]);
  self->contour_count = 0;

  // Read out the union
  for (j = 0; j < cpoly.num_contours; j++)
  {
    ncontour = scan_contour_alloc();
    ncontour->inside = cpoly.hole[j];
    
    for (i = 0; i < cpoly.contour[j].num_vertices; i++)
    {
      point = scan_contour_add_point(ncontour);

      point->x = cpoly.contour[j].vertex[i].x;
      point->y = cpoly.contour[j].vertex[i].y;
    }
    
    scan_solid_append(self, ncontour);
  }

  // Free everything
  gpc_free_polygon(&cpoly);
  gpc_free_polygon(&bpoly);
  gpc_free_polygon(&apoly);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Test to see if a point is inside the solid
int scan_solid_test_inside(scan_solid_t *self, scan_point_t p)
{
  int i;
  int inside;
  scan_contour_t *contour;

  inside = 0;
  
  // The point must lie inside outside contours and outside inside
  // contours
  for (i = 0; i < self->contour_count; i++)
  {
    contour = self->contours[i];
    if (contour->inside)
    {
      if (scan_contour_test_inside(contour, p))
        inside -= 1;
    }
    else
    {
      if (scan_contour_test_inside(contour, p))
        inside += 1;
    }
  }
  return (inside % 2 == 1);
}


////////////////////////////////////////////////////////////////////////////
// Find the line that is nearest the given point.  Returns the
// distance to the line.
double scan_solid_test_nearest(scan_solid_t *self, scan_point_t p,
                               scan_point_t *na, scan_point_t *nb)
{
  int i;
  double min_d, d;
  scan_point_t ma, mb;
  scan_contour_t *contour;

  min_d = DBL_MAX;
  
  // The point must lie outside all of the inside contours and
  // inside at least one of the outside contours.
  for (i = 0; i < self->contour_count; i++)
  {
    contour = self->contours[i];

    d = scan_contour_test_nearest(contour, p, &ma, &mb);

    if (d < min_d)
    {
      min_d = d;

      if (na)
        *na = ma;
      if (nb)
        *nb = mb;
    }
  }
  
  return min_d;
}

