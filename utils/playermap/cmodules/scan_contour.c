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
 * Date: 1 Apr 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

#include "scan.h"


// Some useful macros
#define MIN(x, y) ((x) < (y) ? x : y)
#define MAX(x, y) ((x) > (y) ? x : y)


////////////////////////////////////////////////////////////////////////////
// Create a new contour
scan_contour_t *scan_contour_alloc()
{
  scan_contour_t *self;

  self = calloc(1, sizeof(scan_contour_t));

  self->point_count = 0;
  self->point_max_count = 0;
  self->points = NULL;
  
  return self;
}


////////////////////////////////////////////////////////////////////////////
// Destroy a contour
void scan_contour_free(scan_contour_t *self)
{
  free(self->points);
  free(self);
  return;
}


////////////////////////////////////////////////////////////////////////////
// Create a new contour point
scan_point_t *scan_contour_add_point(scan_contour_t *self)
{
  // Resize array as necessary
  if (self->point_count >= self->point_max_count)
  {
    self->point_max_count += 100;
    self->points = realloc(self->points,
                           self->point_max_count * sizeof(self->points[0]));
  }

  assert(self->point_count < self->point_max_count);
  return self->points + self->point_count++;
}


////////////////////////////////////////////////////////////////////////////
// Delete a contour point
void scan_contour_del_point(scan_contour_t *self, int index)
{
  assert(index >= 0 && index < self->point_count);
  
  memmove(self->points + index, self->points + index + 1,
          (self->point_count - index - 1) * sizeof(self->points[0]));
  self->point_count--;
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Test to see if a point is inside the contour
// The original version of this code was stolen from
// http://astronomy.swin.edu.au/~pbourke/geometry/insidepoly/
int scan_contour_test_inside(scan_contour_t *self, scan_point_t p)
{
  int i;
  double xinters;
  scan_point_t p1, p2;
  int counter = 0;
  
  p1 = self->points[0];
  
  for (i = 1; i <= self->point_count; i++)
  {
    p2 = self->points[i % self->point_count];

    if (p.y > MIN(p1.y, p2.y))
    {
      if (p.y <= MAX(p1.y, p2.y))
      {
        if (p.x <= MAX(p1.x, p2.x))
        {
          if (p1.y != p2.y)
          {
            xinters = (p.y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p.x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  return (counter % 2 == 1);
}


////////////////////////////////////////////////////////////////////////////
// Find the line that is nearest the given point.  Returns the
// distance to the line.
double scan_contour_test_nearest(scan_contour_t *self, scan_point_t p,
                                 scan_point_t *na, scan_point_t *nb)
{
  int i;
  geom_line_t line;
  double d, min_d;
  geom_point_t q, m;
  
  min_d = DBL_MAX;

  q.x = p.x;
  q.y = p.y;
  
  line.pa.x = self->points[0].x;
  line.pa.y = self->points[0].y;
    
  for (i = 1; i <= self->point_count; i++)
  {
    line.pb.x = self->points[i % self->point_count].x;
    line.pb.y = self->points[i % self->point_count].y;    

    d = geom_line_nearest(&line, &q, &m);
    if (d < min_d)
    {
      min_d = d;

      //if (np)
      //  *np = m;
      //if (ni)
      //  *ni = i - 1;

      if (na)
      {
        na->x = line.pa.x;
        na->y = line.pa.y;
      }
      if (nb)
      {
        nb->x = line.pb.x;
        nb->y = line.pb.y;
      }
    }

    line.pa = line.pb;
  }

  return min_d;
}

