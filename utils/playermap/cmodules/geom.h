/***************************************************************************
 * Desc: Useful 2D geometry functions
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id$
 **************************************************************************/

#ifndef GEOM_H
#define GEOM_H


// 2D point
typedef struct
{
  double x, y;
} geom_point_t;


// 2D line segment
typedef struct
{
  geom_point_t pa, pb;
} geom_line_t;


// 2D polygon
typedef struct
{
  int point_count;
  geom_point_t *points;
} geom_polygon_t;


// Compute the normalized angle (-pi to pi)
double geom_normalize(double angle);

// Compute the minimum distance between a line segment and a point
double geom_line_nearest(const geom_line_t *l, const geom_point_t *p, geom_point_t *n);

// Compute intesection between two line segments
// Returns 0 if there is no intersection
int geom_line_test_intersect(const geom_line_t *la, const geom_line_t *lb, geom_point_t *p);


// Create a polygon
geom_polygon_t *geom_polygon_alloc(int point_count);

// Destroy a polygon
void geom_polygon_free(geom_polygon_t *poly);

// Get the nearst point on the polygon; returns the distance
double geom_polygon_nearest(const geom_polygon_t *poly, const geom_point_t *p,
                            geom_point_t *np, int *ni);

// Determine whether or not a point is inside a polygon
int geom_polygon_test_inside(const geom_polygon_t *poly, const geom_point_t *p);

// Compute the area of intersection of two polygons
double geom_polygon_intersect_area(const geom_polygon_t *poly_a, const geom_polygon_t *poly_b);

#endif
