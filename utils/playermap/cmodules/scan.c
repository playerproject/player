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

// Create an approximated free space polygon
static scan_contour_t *scan_create_free(scan_t *self, scan_contour_t *raw,
                                        double err, double maxlen);

// Insert a hit points
static void scan_insert_hits(scan_t *self, scan_contour_t *raw);

// Update hit points that lie inside free space
static void scan_update_hits(scan_t *self, scan_contour_t *raw);



////////////////////////////////////////////////////////////////////////////
// Create a new scan
scan_t *scan_alloc()
{
  scan_t *self;

  self = calloc(1, sizeof(scan_t));

  self->min_range = 0.20;
  self->max_range = 8.00;

  self->free_err = 0.05;
  self->free_len = DBL_MAX;

  self->hit_dist = 0.20;
  
  self->raw = scan_contour_alloc();
  self->free = scan_contour_alloc();
  self->hit = scan_contour_alloc();
  
  return self;
}


////////////////////////////////////////////////////////////////////////////
// Destroy a scan
void scan_free(scan_t *self)
{
  scan_contour_free(self->hit);
  scan_contour_free(self->free);
  scan_contour_free(self->raw);
  
  free(self);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Add range readings to the scan.
void scan_add_ranges(scan_t *self, vector_t pose, int range_count, double ranges[][2])
{
  int i;
  double r, b;
  double ra, rb;
  scan_point_t *point;
  scan_contour_t *raw, *nfree;

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
    
    point->r = r;
    point->b = b;
    point->x = pose.v[0] + r * cos(b + pose.v[2]);
    point->y = pose.v[1] + r * sin(b + pose.v[2]);
  }

  // Store the raw contour
  if (self->raw)
    scan_contour_free(self->raw);
  self->raw = raw;

  // Create an approximated free-space contour
  nfree = scan_create_free(self, raw, self->free_err, self->free_len);

  // Store the free contour
  if (self->free)
    scan_contour_free(self->free);
  self->free = nfree;

  // Insert the new points in the hit list
  scan_insert_hits(self, raw);

  // Update hit points that lie inside free space
  scan_update_hits(self, raw);
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Find the shortest path in a graph
static void scan_reduce_path(scan_t *self, int node_count, scan_node_t *nodes)
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
// Create an approximated free space solid.  This is done using vertex
// elimination.
scan_contour_t *scan_create_free(scan_t *self, scan_contour_t *raw, double err, double maxlen)
{
  int a, b, c;
  double s, d;
  scan_point_t pa, pb, pc;
  geom_point_t qa, qb, qc;
  geom_line_t line;
  scan_node_t *nodes, *node;
  scan_point_t *point;
  scan_contour_t *nfree;
  
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
    for (b = a - 2; b >= MAX(0, a - 10); b--) // HACK
//    for (b = a - 2; b >= 0; b--)
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
  nfree = scan_contour_alloc();
  
  // Extract the path we are interested in.
  a = 0;
  while (a >= 0)
  {
    node = nodes + a;
    b = node->next;

    point = scan_contour_add_point(nfree);
    
    //point->w = b - a;
    point->w = 1.0;
    assert(point->w > 0);
    point->r = raw->points[a].r;
    point->b = raw->points[a].b;
    point->x = raw->points[a].x;
    point->y = raw->points[a].y;

    a = b;
  }

  // Clean up
  for (a = 0; a < raw->point_count; a++)
  {
    node = nodes + a;
    free(node->arcs);
  }
  free(nodes);
  
  return nfree;
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
    
    for (j = 0; j < self->hit->point_count; j++)
    {
      npoint = self->hit->points + j;

      dx = npoint->x - point->x;
      dy = npoint->y - point->y;
      dr = sqrt(dx * dx + dy * dy);

      if (dr < self->hit_dist)
        break;
    }

    if (j < self->hit->point_count)
    {
      npoint = self->hit->points + j;
      npoint->w += 1;
    }
    else
    {
      npoint = scan_contour_add_point(self->hit);
      npoint->x = point->x;
      npoint->y = point->y;
      npoint->w = 1;
    }
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Update hit points that lie inside free space
void scan_update_hits(scan_t *self, scan_contour_t *raw)
{
  int i;
  double d;
  scan_point_t *point;
  
  for (i = 0; i < self->hit->point_count; i++)
  {
    point = self->hit->points + i;
    if (point->w > 0)
    {
      if (scan_contour_test_inside(raw, *point))
      {
        d = scan_contour_test_nearest(raw, *point, NULL, NULL);
        point->w -= d / 0.20;
      }
    }
  }
  return;
}


////////////////////////////////////////////////////////////////////////////
// Test a point to see if it lies within free space
int scan_test_free(scan_t *self, scan_point_t point)
{
  return scan_contour_test_inside(self->free, point);
}


////////////////////////////////////////////////////////////////////////////
// Test a point to see if it lies within occupied space
int scan_test_occ(scan_t *self, scan_point_t point, double dist)
{
  int j;
  double dx, dy, dr;
  scan_point_t *npoint;
  
  // TODO: REALLY SLOW
  for (j = 0; j < self->hit->point_count; j++)
  {
    npoint = self->hit->points + j;

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

