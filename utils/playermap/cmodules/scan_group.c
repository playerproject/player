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
 * Desc: Range scan grouping
 * Author: Andrew Howard
 * Date: 14 Sep 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>

//#include <values.h>
#include <limits.h> // POSIX replacement - rtv
#include <float.h>  //  "

#include "scan.h"

////////////////////////////////////////////////////////////////////////////
// Local functions
static void scan_group_update_free(scan_group_t *self, vector_t pose, scan_t *scan);
static void scan_group_update_hits(scan_group_t *self, vector_t pose, scan_t *scan);


////////////////////////////////////////////////////////////////////////////
// Create a new scan group
scan_group_t *scan_group_alloc()
{
  scan_group_t *self;

  self = calloc(1, sizeof(scan_group_t));

  self->hit_dist = 0.10;
    
  self->free = scan_solid_alloc();
  self->hits = scan_contour_alloc();

  return self;
}


////////////////////////////////////////////////////////////////////////////
// Destroy a scan group
void scan_group_free(scan_group_t *self)
{
  scan_contour_free(self->hits);
  scan_solid_free(self->free);
  free(self);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Reset the scan group (discards all scans)
void scan_group_reset(scan_group_t *self)
{
  scan_solid_reset(self->free);
  scan_contour_reset(self->hits);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Add a scan to the group
void scan_group_add(scan_group_t *self, vector_t pose, scan_t *scan)
{
  // Add scan to free solid
  scan_group_update_free(self, pose, scan);

  // Add scan to hit points
  scan_group_update_hits(self, pose, scan);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Add a scan to the free solid
static void scan_group_update_free(scan_group_t *self, vector_t pose, scan_t *scan)
{
  // Take union with existing free solid
  scan_solid_union(self->free, pose, scan->free);

  // TODO: approximate the solid
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Add a scan to the hit list
static void scan_group_update_hits(scan_group_t *self, vector_t pose, scan_t *scan)
{
  int i, j;
  double ax, ay, dx, dy;
  scan_point_t *point, *npoint;
  
  for (i = 0; i < scan->hits->point_count; i++)
  {
    point = scan->hits->points + i;
    ax = pose.v[0] + point->x * cos(pose.v[2]) - point->y * sin(pose.v[2]);
    ay = pose.v[1] + point->x * sin(pose.v[2]) + point->y * cos(pose.v[2]);
    
    for (j = 0; j < self->hits->point_count; j++)
    {
      npoint = self->hits->points + j;      
      dx = ax - npoint->x;
      dy = ay - npoint->y;

      if (sqrt(dx * dx + dy * dy) < self->hit_dist)
        break;
    }

    if (j < self->hits->point_count)
    {
      npoint = self->hits->points + j;      
      npoint->w += 1;
    }
    else
    {
      npoint = scan_contour_add_point(self->hits);
      npoint->x = ax;
      npoint->y = ay;
      npoint->w = 1;
    }
  }
  return;
}



