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
 * Desc: Range scan matching
 * Author: Andrew Howard
 * Date: 1 Apr 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <values.h>
#include <gsl/gsl_multifit_nlin.h>

#include "scan.h"


////////////////////////////////////////////////////////////////////////////
// Create a new scan match
scan_match_t *scan_match_alloc(scan_group_t *scan_a, scan_group_t *scan_b)
{
  scan_match_t *self;

  self = calloc(1, sizeof(scan_match_t));
  self->scan_a = scan_a;
  self->scan_b = scan_b;

  self->pair_count = 0;
  self->pair_max_count = scan_a->hits->point_count + scan_b->hits->point_count;
  self->pairs = calloc(self->pair_max_count, sizeof(self->pairs[0]));

  return self;
}


////////////////////////////////////////////////////////////////////////////
// Destroy a scan match
void scan_match_free(scan_match_t *self)
{
  free(self->pairs);
  free(self);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Generate correspondance points
void scan_match_pairs(scan_match_t *self, vector_t pose_a, vector_t pose_b)
{
  int i;
  double cos_a, sin_a, cos_b, sin_b;
  scan_point_t sa, sb1, sb2;
  scan_point_t sb, sa1, sa2;
  scan_pair_t *pair;
  scan_point_t p, q;
  double s;

  cos_a = cos(pose_a.v[2]);
  sin_a = sin(pose_a.v[2]);

  cos_b = cos(pose_b.v[2]);
  sin_b = sin(pose_b.v[2]);
  
  self->pair_count = 0;

  // For each hit point in A...
  for (i = 0; i < self->scan_a->hits->point_count; i++)
  {
    sa = self->scan_a->hits->points[i];

    // Compute point in global cs
    p.x = pose_a.v[0] + sa.x * cos_a - sa.y * sin_a;
    p.y = pose_a.v[1] + sa.x * sin_a + sa.y * cos_a;

    // Compute point in B's local cs
    q.x = +(p.x - pose_b.v[0]) * cos_b + (p.y - pose_b.v[1]) * sin_b;
    q.y = -(p.x - pose_b.v[0]) * sin_b + (p.y - pose_b.v[1]) * cos_b;
    
    // See if A's hit point is inside the free space solid for B...
    if (scan_solid_test_inside(self->scan_b->free, q))
    {
      // Get the nearest line segment
      s = scan_solid_test_nearest(self->scan_b->free, q, &sb1, &sb2);
      if (s > self->outlier_dist)
        continue;
      
      assert(self->pair_count < self->pair_max_count);
      pair = self->pairs + self->pair_count++;
      
      pair->type = 1;
      pair->w = sa.w;
      pair->ia = i;
      pair->ib = -1;
      pair->pa.x = sa.x;
      pair->pa.y = sa.y;
      pair->lb.pa.x = sb1.x;
      pair->lb.pa.y = sb1.y;
      pair->lb.pb.x = sb2.x;
      pair->lb.pb.y = sb2.y;
    }
  }

  // For each hit point in B...
  for (i = 0; i < self->scan_b->hits->point_count; i++)
  {
    sb = self->scan_b->hits->points[i];

    // Compute point in global cs
    p.x = pose_b.v[0] + sb.x * cos_b - sb.y * sin_b;
    p.y = pose_b.v[1] + sb.x * sin_b + sb.y * cos_b;

    // Compute point in A's local cs
    q.x = +(p.x - pose_a.v[0]) * cos_a + (p.y - pose_a.v[1]) * sin_a;
    q.y = -(p.x - pose_a.v[0]) * sin_a + (p.y - pose_a.v[1]) * cos_a;
    
    // See if A's hit point is inside the free space solid for B...
    if (scan_solid_test_inside(self->scan_a->free, q))
    {
      // Get the nearest line segment
      s = scan_solid_test_nearest(self->scan_a->free, q, &sa1, &sa2);
      if (s > self->outlier_dist)
        continue;
      
      assert(self->pair_count < self->pair_max_count);
      pair = self->pairs + self->pair_count++;

      pair->type = 2;
      pair->ia = -1;
      pair->ib = i;
      pair->w = sb.w;
      pair->la.pa.x = sa1.x;
      pair->la.pa.y = sa1.y;
      pair->la.pb.x = sa2.x;
      pair->la.pb.y = sa2.y;
      pair->pb.x = sb.x;
      pair->pb.y = sb.y;
    }
  }

  return;
}


