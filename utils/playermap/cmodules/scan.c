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
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <values.h>

#ifndef MIN 
#define MIN(a,b) (((a)<(b))?(a):(b)) 
#endif 

#ifndef MAX 
#define MAX(a,b) (((a)>(b))?(a):(b)) 
#endif

#include "scan.h"


// Graph node data for scan approximation
typedef struct
{
  // Arcs eminating from this node
  int arc_count;
  int *arcs;

  // The next node in the path
  int next;

  // The distance to the goal
  double dist;

} scan_node_t;


// Find the shortest path in the graph
static void scan_reduce_path(scan_t *self, int node_count, scan_node_t *nodes);

// Create an approximated contour
static void scan_create_contour(scan_t *self, scan_contour_t *raw,
                                double err, double maxlen);

// Insert a hit points
static void scan_insert_hits(scan_t *self, scan_contour_t *raw);


////////////////////////////////////////////////////////////////////////////
// Create a new scan
scan_t *scan_alloc()
{
  scan_t *self;

  self = calloc(1, sizeof(scan_t));

  self->min_range = 0.20;
  self->max_range = 8.00;

  self->free_points = 10;
  self->free_err = 0.05;
  self->free_len = DBL_MAX;
  
  self->hit_dist = 0.15;
  
  self->raw = scan_contour_alloc();
  self->free = scan_contour_alloc();
  self->hits = scan_contour_alloc();
  self->sites = scan_contour_alloc();
  
  return self;
}


////////////////////////////////////////////////////////////////////////////
// Destroy a scan
void scan_free(scan_t *self)
{
  scan_contour_free(self->sites);
  scan_contour_free(self->hits);
  scan_contour_free(self->free);
  scan_contour_free(self->raw);
  
  free(self);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Add range readings to the scan.
int scan_add_ranges(scan_t *self, vector_t pose, int range_count, double ranges[][2])
{
  int i;
  int reject;
  double r, b;
  double ra, rb;
  scan_point_t *point;
  scan_contour_t *raw;

  reject = 0;
  
  // Create a new raw contour
  raw = scan_contour_alloc();
 
  // Add laser points to the contour
  for (i = 0; i < range_count; i++)
  {
    point = scan_contour_add_point(raw);

    r = ranges[i][0];
    b = ranges[i][1];

    // Do some pre-filtering for long readings
    if (r > self->max_range)
    {
      if (i - 1 >= 0 && i + 1 < range_count)
      {
        ra = ranges[i - 1][0];
        rb = ranges[i + 1][0];

        if (ra < self->max_range && rb < self->max_range)
          r = (ra + rb) / 2;
        else
          r = self->max_range + 1e-6;
      }
      else
        r = self->max_range + 1e-6;
    }

    // If any point is short, reject the whole scan
    if (r < self->min_range)
      reject = 1;
    
    point->r = r;
    point->b = b;
    point->x = pose.v[0] + r * cos(b + pose.v[2]);
    point->y = pose.v[1] + r * sin(b + pose.v[2]);
  }

  // Store the raw contour
  if (self->raw)
    scan_contour_free(self->raw);
  self->raw = raw;

  // We may have to reject the entire scan
  if (reject)
  {
    scan_contour_reset(self->free);
    scan_contour_reset(self->hits);
    return -1;    
  }
  
  // Create approximated contours
  scan_create_contour(self, raw, self->free_err, self->free_len);

  // Insert the new points in the hit list
  scan_contour_reset(self->hits);
  scan_insert_hits(self, raw);
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Create an approximated free space contour.  This is done using
// vertex elimination.
void scan_create_contour(scan_t *self, scan_contour_t *raw, double err, double maxlen)
{
  int i, j;
  int a, b, c;
  int valid;
  double s, d;
  scan_point_t pa, pb, pc;
  geom_point_t qa, qb, qc;
  geom_line_t line;
  scan_node_t *nodes, *node;
  scan_point_t *point;
  
  // Allocate space for the graph
  nodes = calloc(raw->point_count, sizeof(nodes[0]));

  // Initialize the graph; this is O(n^3)
  // Note that we construct the graph "backwards" to
  // make it easier to extract the shortest path.
  for (a = raw->point_count - 1; a >= 0; a--)
  {
    pa = raw->points[a];
    qa.x = pa.x;
    qa.y = pa.y;

    node = nodes + a;
    node->arc_count = 0;
    node->arcs = calloc(raw->point_count, sizeof(node->arcs[0]));
    node->next = -1;
    node->dist = INT_MAX;

    // Always add an arc to the next node
    if (a > 0)
      node->arcs[node->arc_count++] = a - 1;

    // Test other posibilities
    for (b = a - 2; b >= MAX(0, a - self->free_points); b--) // HACK
    {
      assert(a != b);
      assert(b >= 0);
      
      pb = raw->points[b];
      qb.x = pb.x;
      qb.y = pb.y;

      // REMOVE
      // Cant combine out-of-range readings with regular readings
      //if (pa.r < self->max_range && pb.r >= self->max_range)
      //  continue;
      //if (pa.r >= self->max_range && pb.r < self->max_range)
      //  continue;

      // Cant combine points if they are too far apart
      d = sqrt((qb.x - qa.x) * (qb.x - qa.x) + (qb.y - qa.y) * (qb.y - qa.y));
      if (d > maxlen)
        continue;

      line.pa = qa;
      line.pb = qb;

      // Test the arc to see if it is ok (doesnt exceed error bound)
      s = 0;
      for (c = a - 1; c > b; c--)
      {
        assert(c >= 0);
        pc = raw->points[c];        
        qc.x = pc.x;
        qc.y = pc.y;
        s = geom_line_nearest(&line, &qc, NULL);
        if (s > err)
          break;
      }

      // This arc is ok, so add it to the graph
      if (s < err)
        node->arcs[node->arc_count++] = b;
      assert(node->arc_count <= raw->point_count);
    }
  }

  // Find the shortest path in the graph
  scan_reduce_path(self, raw->point_count, nodes);

  // Create a new contour
  scan_contour_reset(self->free);
  
  // Extract the path we are interested in
  a = 0;
  while (a >= 0)
  {
    node = nodes + a;
    b = node->next;

    point = scan_contour_add_point(self->free);
    
    //point->w = b - a;
    point->w = 1.0;
    assert(point->w > 0);
    point->r = raw->points[a].r;
    point->b = raw->points[a].b;
    point->x = raw->points[a].x;
    point->y = raw->points[a].y;

    //printf("%d %d\n", a, b);

    /* REMOVE?
    if (b < 0)
      break;

    // See if this looks like a line segment
    valid = 0;
    for (i = a; i < b;)
    {
      if (raw->points[i].r > self->max_range)
      {
        i++;
        continue;
      }
      
      // Find a contiguous set of hit points in this interval
      for (j = i + 1; j <= b; j++)
      {
        if (raw->points[j].r > self->max_range)
          break;

        pa = raw->points[j - 1];
        pb = raw->points[j];
        d = sqrt((pb.x - pa.x) * (pb.x - pa.x) + (pb.y - pa.y) * (pb.y - pa.y));
        if (d > 0.40) // HACK
          break;
      }
      j--;

      // Need a few points to make a contiguous set
      if (j - i + 1 < 4)
      {
        i = j + 1;
        continue;
      }

      // Segment needs to be long enough
      pa = raw->points[i];
      pb = raw->points[j];
      d = sqrt((pb.x - pa.x) * (pb.x - pa.x) + (pb.y - pa.y) * (pb.y - pa.y));
      if (d > 0.40) // HACK
      {
        valid = 1;
        break;
      }
      
      i = j + 1;
    }

    // Add points to the list, eliminate dups as we go
    if (valid)
    {
      for (i = a; i <= b; i++)
      {
        if (i > a)
        {
          pb = raw->points[i];
          d = sqrt((pb.x - pa.x) * (pb.x - pa.x) + (pb.y - pa.y) * (pb.y - pa.y));
          if (d < self->hit_dist)
            continue;
        }
          
        pa = raw->points[i];          
        point = scan_contour_add_point(self->hits);
        point->w = 1.0;
        point->r = pa.r;
        point->b = pa.b;
        point->x = pa.x;
        point->y = pa.y;
      }
    }
    */

    a = b;
  }

  // Clean up
  for (a = 0; a < raw->point_count; a++)
  {
    node = nodes + a;
    free(node->arcs);
  }
  free(nodes);

  return;
}


////////////////////////////////////////////////////////////////////////////
// Insert a hit points.  This does some clustering to reduce the total number
// of points.
void scan_insert_hits(scan_t *self, scan_contour_t *raw)
{
  int i, j;
  scan_point_t *point, *npoint;
  double dx, dy, dr;

  // The nearest neighbor test is O(n^2)
  for (i = 0; i < raw->point_count; i++)
  {
    point = raw->points + i;

    // Ignore invalid readings
    if (point->r > self->max_range || point->r <= self->min_range)
      continue;
    
    for (j = 0; j < self->hits->point_count; j++)
    {
      npoint = self->hits->points + j;

      dx = npoint->x - point->x;
      dy = npoint->y - point->y;
      dr = sqrt(dx * dx + dy * dy);

      if (dr < self->hit_dist)
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
      npoint->x = point->x;
      npoint->y = point->y;
      npoint->w = 1;
    }
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Find the shortest path in a graph
void scan_reduce_path(scan_t *self, int node_count, scan_node_t *nodes)
{
  int i, a, b;
  scan_node_t *node_a, *node_b;
  int *queue;
  int queue_start, queue_end;
  
  // Create a queue
  queue_start = 0;
  queue_end = 0;
  queue = calloc(node_count, sizeof(queue[0]));

  // Initialize the queue
  a = node_count - 1;
  node_a = nodes + a;
  node_a->dist = 0;
  node_a->next = -1;
  queue[queue_end++] = a;

  // Find all paths
  while (queue_end > queue_start)
  {
    a = queue[queue_start++];
    assert(a >= 0);
    node_a = nodes + a;

    for (i = 0; i < node_a->arc_count; i++)
    {
      b = node_a->arcs[i];      
      node_b = nodes + b;

      assert(b >= 0);
      assert(a != b);

      if (node_b->dist > node_a->dist + 1)
      {        
        node_b->dist = node_a->dist + 1;
        node_b->next = a;
        assert(queue_end < node_count);
        queue[queue_end++] = b;

        if (b == 0)
          break;
      }
    }
  }
  
  free(queue);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Test a point to see if it lies within free space; if ourside free
// space, return -1, if inside free space, returns the distance to the
// nearest boundary
double scan_test_free(scan_t *self, scan_point_t point)
{
  if (!scan_contour_test_inside(self->free, point))
    return -1;

  return scan_contour_test_nearest(self->free, point, NULL, NULL);
}


////////////////////////////////////////////////////////////////////////////
// Test a line to see if it lies entire within free space
int scan_test_free_line(scan_t *self, scan_point_t pa, scan_point_t pb)
{
  // Quick test: does either endpoint lie outside free space?
  if (scan_test_free(self, pa) < 0)
    return 0;
  if (scan_test_free(self, pb) < 0)
    return 0;

  return (scan_contour_test_line_intersect(self->free, pa, pb) == 0);
}


////////////////////////////////////////////////////////////////////////////
// Test a point to see if it lies within occupied space
int scan_test_occ(scan_t *self, scan_point_t point, double dist)
{
  int j;
  double dx, dy, dr;
  scan_point_t *npoint;
  
  // TODO: REALLY SLOW
  for (j = 0; j < self->hits->point_count; j++)
  {
    npoint = self->hits->points + j;

    if (npoint->w > 0)
    {
      dx = npoint->x - point.x;
      dy = npoint->y - point.y;
      dr = sqrt(dx * dx + dy * dy);

      if (dr < dist)
        return 1;
    }
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////
// Generate sites within the scan
int scan_make_sites(scan_t *self, int max_attempts)
{
  int i, j;
  int inside, attempt;
  double dx, dy;
  double r, b, radius;
  scan_point_t *point;
  scan_point_t test_point;

  double cspace_dist = 0.25; // HACK
  
  scan_contour_reset(self->sites);

  for (attempt = 0; attempt < max_attempts; attempt++)
  {
    // Pick a range and bearing at random
    i = (int) (((double) self->raw->point_count * rand()) / (RAND_MAX + 1.0));
    assert(i >= 0 && i < self->raw->point_count);
    point = self->raw->points + i;
    r = (point->r * (double) rand()) / RAND_MAX;
    b = point->b;

    // TODO: offset?
    test_point.x = r * cos(b);
    test_point.y = r * sin(b);

    //printf("%d %d %d %f %f\n", attempt, i, rand() % 181, test_point.x, test_point.y);
    
    // Compute site radius
    radius = scan_test_free(self, test_point);
    radius -= cspace_dist;

    if (radius < 0.10) // HACK
      continue;

    // Make sure this doesnt lie within an existing site
    inside = 0;
    for (j = 0; j < self->sites->point_count; j++)
    {
      point = self->sites->points + j;
      dx = point->x - test_point.x;
      dy = point->y - test_point.y;

      if (sqrt(dx * dx + dy * dy) < point->r)
      {
        inside = 1;
        break;
      }
    }
    if (inside)
      continue;

    // Add to the site list
    point = scan_contour_add_point(self->sites);
    point->x = test_point.x;
    point->y = test_point.y;
    point->r = radius;
  }

  return self->sites->point_count;
}


////////////////////////////////////////////////////////////////////////////
// Suppress sites that overlap the given site
void scan_suppress_sites(scan_t *self, vector_t pose, double radius)
{
  int i;
  double dx, dy;
  scan_point_t *site;
  
  for (i = 0; i < self->sites->point_count; i++)
  {
    site = self->sites->points + i;
    dx = site->x - pose.v[0];
    dy = site->y - pose.v[1];
    if (sqrt(dx * dx + dy * dy) < radius)
      site->r = -1.0;
  }
  
  return;
}
