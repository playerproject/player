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
 * Date: 1 April 2003
 * CVS: $Id$
 **************************************************************************/

#ifndef SCAN_H
#define SCAN_H

#include "vector.h"
#include "geom.h"


/**************************************************************************
 * Scan data structures
 **************************************************************************/

// Scan point
typedef struct _scan_point_t
{  
  // Range and bearing relative to sensor
  double r, b;

  // Cartesian coords relative to scan origin
  double x, y;

  // Weight value (the number of points in the original scan that
  // mapped onto this reduced point)
  double w;

} scan_point_t;


/**************************************************************************
 * Scan contour functions
 **************************************************************************/

// A scan contour
typedef struct _scan_contour_t
{
  // Contour is either inside or outside contour
  int inside;
  
  // Number of point in the contour
  int point_count, point_max_count;

  // The points
  scan_point_t *points;
  
} scan_contour_t;


// Create a new contour
scan_contour_t *scan_contour_alloc();

// Destroy a contour
void scan_contour_free(scan_contour_t *self);

// Empty points from the contour
void scan_contour_reset(scan_contour_t *self);

// Append a point to a contour
scan_point_t *scan_contour_add_point(scan_contour_t *self);

// Remove a point from a contour
void scan_contour_del_point(scan_contour_t *self, int index);

// Test to see if a point is inside the contour
int scan_contour_test_inside(scan_contour_t *self, const scan_point_t p);

// Find the line that is nearest the given point.  Returns the
// distance to the line.
double scan_contour_test_nearest(scan_contour_t *self, const scan_point_t p,
                                 scan_point_t *na, scan_point_t *nb);

// TODO: make this test for nearness rather than intersection
// See if the given line intersects the contour 
int scan_contour_test_line_intersect(scan_contour_t *self,
                                     scan_point_t pa, scan_point_t pb);



/**************************************************************************
 * Scan solid functions
 **************************************************************************/

// A scan solid
typedef struct _scan_solid_t
{  
  // Number of contour in the contour
  int contour_count, contour_max_count;

  // The contours
  scan_contour_t **contours;
  
} scan_solid_t;


// Create a new solid
scan_solid_t *scan_solid_alloc();

// Destroy a solid
void scan_solid_free(scan_solid_t *self);

// Reset a solid to empty
void scan_solid_reset(scan_solid_t *self);

// Take union of solid with a contour
void scan_solid_union(scan_solid_t *self, vector_t pose, scan_contour_t *contour);

// Test to see if a point is inside the solid
int scan_solid_test_inside(scan_solid_t *self, scan_point_t p);

// Find the line that is nearest the given point.  Returns the
// distance to the line.
double scan_solid_test_nearest(scan_solid_t *self, scan_point_t p,
                               scan_point_t *na, scan_point_t *nb);



/**************************************************************************
 * Scan functions
 **************************************************************************/

// Range scan data
typedef struct _scan_t
{
  // Max valid range value
  double min_range, max_range;

  // Free-space contour extraction
  double free_points, free_err, free_len;

  // Hit point clustering distance
  double hit_dist;
  
  // Raw scan
  scan_contour_t *raw;

  // Free space contour
  scan_contour_t *free;

  // Hit list
  scan_contour_t *hits;

  // Site list
  scan_contour_t *sites;

} scan_t;


// Create a new scan
scan_t *scan_alloc();

// Destroy a scan
void scan_free(scan_t *self);

// Add range readings to the scan
int scan_add_ranges(scan_t *self, vector_t pose, int range_count, double ranges[][2]);

// Test a point to see if it lies within free space; if ourside free
// space, return -1, if inside free space, returns the distance to the
// nearest boundary
double scan_test_free(scan_t *self, scan_point_t point);

// Test a line to see if it lies entire within free space
int scan_test_free_line(scan_t *self, scan_point_t pa, scan_point_t pb);

// Test a point to see if it lies within occupied space
int scan_test_occ(scan_t *self, scan_point_t point, double dist);


/**************************************************************************
 * Scan group functions
 **************************************************************************/

// Scan group object
typedef struct _scan_group_t
{
  // Hit point clustering distance
  double hit_dist;

  // Free-space polysolid (approximated)
  scan_solid_t *free;

  // Hit point list (clustered)
  scan_contour_t *hits;
  
} scan_group_t;


// Create a new scan group
scan_group_t *scan_group_alloc();

// Destroy a scan group
void scan_group_free(scan_group_t *self);

// Reset the scan group (discards all scans)
void scan_group_reset(scan_group_t *self);

// Add a scan to the group
void scan_group_add(scan_group_t *self, vector_t pose, scan_t *scan);



/**************************************************************************
 * Scan matching functions
 **************************************************************************/

// Point pairs
typedef struct _scan_pair_t
{
  // Pair type (point-point, point-line, line-point)
  int type;

  // Point indices
  int ia, ib;

  // Weight
  double w;

  // Point or line pairs
  geom_point_t pa, pb;
  geom_line_t la, lb;
  
} scan_pair_t;


// Scan match data
typedef struct _scan_match_t
{
  // The two scans being compared
  scan_group_t *scan_a;
  scan_group_t *scan_b;
  
  // Point pairs
  int pair_count, pair_max_count;
  scan_pair_t *pairs;

  // Outlier distance
  double outlier_dist;
  
} scan_match_t;


// Create a scan match
scan_match_t *scan_match_alloc(scan_group_t *scan_a, scan_group_t *scan_b);

// Destroy a scan match
void scan_match_free(scan_match_t *self);

// Generate point-pairs for the two scans
void scan_match_pairs(scan_match_t *self, vector_t pose_a, vector_t pose_b);

#endif
