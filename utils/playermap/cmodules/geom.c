/* 
 *  Mezzanine - an overhead visual object tracker.
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Geometry functions
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id$
 **************************************************************************/

#include <assert.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "geom.h"


// Some useful macros
#define MIN(x, y) ((x) < (y) ? x : y)
#define MAX(x, y) ((x) > (y) ? x : y)

// Declarations
static double pos_inter(geom_point_t * a, int na, geom_point_t * b, int nb);


// Compute the normalized angle (-pi to pi)
double geom_normalize(double angle)
{
  return atan2(sin(angle), cos(angle));
}


// Compute the minimum distance between a line and a point
double geom_line_nearest(const geom_line_t *l, const geom_point_t *p, geom_point_t *n)
{
  double a, b, d, s;
  double nx, ny;
  double dx, dy;

	// Compute parametric intercept point
  a = (l->pb.x - l->pa.x);
  b = (l->pb.y - l->pa.y);
  d = a * a + b * b;

  if (d < 1e-16)
    s = 0;
  else
    s = (a * (p->x - l->pa.x) + b * (p->y - l->pa.y)) / d;
  
  if (!finite(s))
  {
    printf("%f %f %f\n", a, b, s);
    assert(finite(s));
  }

	// Bound to lie between endpoints
	if (s < 0)
		s = 0;
	if (s > 1)
		s = 1;

	// Compute intercept point
  nx = l->pa.x + a * s;
  ny = l->pa.y + b * s;
  
  if (n)
  {
    n->x = nx;
    n->y = ny;
  }

	// Compute distance
  dx = nx - p->x;
  dy = ny - p->y;
  return sqrt(dx * dx + dy * dy);
}


// Compute intesection between two line segments
// Returns 0 if there is no intersection
int geom_line_test_intersect(const geom_line_t *la, const geom_line_t *lb, geom_point_t *p)
{
  double a11, a12, a21, a22;
  double b11, b12, b21, b22;
  double s, t;
  
	a11 = (la->pb.x - la->pa.x);
	b11 = la->pa.x;
	a12 = (la->pb.y - la->pa.y);
	b12 = la->pa.y;

  a21 = (lb->pb.x - lb->pa.x);
	b21 = lb->pa.x;
	a22 = (lb->pb.y - lb->pa.y);
	b22 = lb->pa.y;

	// See if there is an intercept
	if (fabs(a12 * a21 - a11 * a22) < 1e-16)
		return 0;
	
	// Compute parmetric intercepts
	s = ((a22 * b11 - a21 * b12) - (a22 * b21 - a21 * b22))
    / (a12 * a21 - a11 * a22);
	t = ((a12 * b11 - a11 * b12) - (a12 * b21 - a11 * b22)) 
    / (a12 * a21 - a11 * a22);
  
	// See if there is an intercept within the segment
	if (s < 0 || s > 1)
		return 0;
	if (t < 0 || t > 1)
		return 0;

	// Compute the intercept
	if (p != NULL)
	{
		p->x = a11 * s + b11;
		p->y = a12 * s + b12;
	}
	return 1;
}


// Create a polygon
geom_polygon_t *geom_polygon_alloc(int point_count)
{
  geom_polygon_t *poly;

  poly = calloc(1, sizeof(geom_polygon_t));
  poly->point_count = point_count;
  poly->points = calloc(point_count, sizeof(geom_point_t));
  
  return poly;
}


// Destroy a polygon
void geom_polygon_free(geom_polygon_t *poly)
{
  free(poly->points);
  free(poly);
  return;
}


// Get the nearst point on the polygon; returns the distance
double geom_polygon_nearest(const geom_polygon_t *poly, const geom_point_t *p,
                            geom_point_t *np, int *ni)
{
  int i;
  geom_line_t line;
  double d, min_d;
  geom_point_t m;
  
  min_d = DBL_MAX;

  line.pa = poly->points[0];
  for (i = 1; i <= poly->point_count; i++)
  {
    line.pb = poly->points[i % poly->point_count];

    d = geom_line_nearest(&line, p, &m);
    if (d < min_d)
    {
      min_d = d;

      if (np)
        *np = m;
      if (ni)
        *ni = i - 1;
    }

    line.pa = line.pb;
  }

  return min_d;
}


// Determine whether or not a point is inside a polygon
// The original version of this code was stolen from
// http://astronomy.swin.edu.au/~pbourke/geometry/insidepoly/
int geom_polygon_test_inside(const geom_polygon_t *poly, const geom_point_t *p)
{
  int i;
  double xinters;
  geom_point_t p1, p2;
  int counter = 0;
  
  p1 = poly->points[0];
  
  for (i = 1; i <= poly->point_count; i++)
  {
    p2 = poly->points[i % poly->point_count];

    if (p->y > MIN(p1.y, p2.y))
    {
      if (p->y <= MAX(p1.y, p2.y))
      {
        if (p->x <= MAX(p1.x, p2.x))
        {
          if (p1.y != p2.y)
          {
            xinters = (p->y-p1.y)*(p2.x-p1.x)/(p2.y-p1.y)+p1.x;
            if (p1.x == p2.x || p->x <= xinters)
              counter++;
          }
        }
      }
    }
    p1 = p2;
  }

  return (counter % 2 == 1);
}


// Compute the area of intersection of two polygons
double geom_polygon_intersect_area(const geom_polygon_t *poly_a, const geom_polygon_t *poly_b)
{
  double area;

  area = pos_inter(poly_a->points, poly_a->point_count, poly_b->points, poly_b->point_count);

  return area;
}


// Compute the area of intersection between two polygons
// This disgraceful P.O.S was snarfed from here: http://www.cap-lore.com/MathPhys/IP/
// Some folks shouldnt be allowed to code in C.
static double pos_inter(geom_point_t * a, int na, geom_point_t * b, int nb)
{
  typedef struct{geom_point_t min; geom_point_t max;} box;
  typedef long long hp;
  typedef struct{long x; long y;} ipoint;
  typedef struct{long mn; long mx;} rng;
  typedef struct{ipoint ip; rng rx; rng ry; short in;} vertex;
  
  vertex ipa[na+1], ipb[nb+1];
  box B = {{DBL_MAX, DBL_MAX}, {-DBL_MAX, -DBL_MAX}};
  double ascale;

  void range(geom_point_t * x, int c)
    {
      while(c--)
      {
        void bd(double * X, double y){*X = *X<y ? *X:y;}
        void bu(double * X, double y){*X = *X>y ? *X:y;}
      
        bd(&B.min.x, x[c].x); bu(&B.max.x, x[c].x);
        bd(&B.min.y, x[c].y); bu(&B.max.y, x[c].y);
      }
    }
  
  if(na < 3 || nb < 3)
    return 0;
  range(a, na); range(b, nb);
  
  {
    const double gamut = 500000000., mid = gamut/2.;
    double rngx = B.max.x - B.min.x, sclx = gamut/rngx;
    double rngy = B.max.y - B.min.y, scly = gamut/rngy;
    
    {
      void fit(geom_point_t * x, int cx, vertex * ix, int fudge)
        {
          {
            int c=cx; while(c--)
            {
              ix[c].ip.x = (long)((x[c].x - B.min.x)*sclx - mid)&~7|fudge|c&1;
              ix[c].ip.y = (long)((x[c].y - B.min.y)*scly - mid)&~7|fudge;
            }
          }
          ix[0].ip.y += cx&1;
          ix[cx] = ix[0];
          {
            int c=cx; while(c--)
            {
              ix[c].rx = ix[c].ip.x < ix[c+1].ip.x ?
                (rng){ix[c].ip.x,ix[c+1].ip.x}:(rng){ix[c+1].ip.x,ix[c].ip.x};
              ix[c].ry = ix[c].ip.y < ix[c+1].ip.y ?
                (rng){ix[c].ip.y,ix[c+1].ip.y}:(rng){ix[c+1].ip.y,ix[c].ip.y};
              ix[c].in=0;
            }
          }
        }

      fit(a, na, ipa, 0); fit(b, nb, ipb, 2);
    }

    ascale = sclx*scly;
  }
  {
    hp area(ipoint a, ipoint p, ipoint q)
      {
        return (hp)p.x*q.y - (hp)p.y*q.x +
          (hp)a.x*(p.y - q.y) + (hp)a.y*(q.x - p.x);
      }

    hp s = 0; int j, k;

    void cntrib(ipoint f, ipoint t, short w, char * S)
      {
        s += (hp)w*(t.x-f.x)*(t.y+f.y)/2;
      }

    int ovl(rng p, rng q)
      {
        return p.mn < q.mx && q.mn < p.mx;
      }

    for(j=0; j<na; ++j) for(k=0; k<nb; ++k)
      if(ovl(ipa[j].rx, ipb[k].rx) && ovl(ipa[j].ry, ipb[k].ry))
      {
        hp a1 = -area(ipa[j].ip, ipb[k].ip, ipb[k+1].ip),
          a2 = area(ipa[j+1].ip, ipb[k].ip, ipb[k+1].ip);
        {
          int o = a1<0; if(o == a2<0)
          {
            hp a3 = area(ipb[k].ip, ipa[j].ip, ipa[j+1].ip),
              a4 = -area(ipb[k+1].ip, ipa[j].ip, ipa[j+1].ip);
            if(a3<0 == a4<0)
            {
              void cross(vertex * a, vertex * b, vertex * c, vertex * d,
                         double a1, double a2, double a3, double a4)
                {
                  double r1=a1/((double)a1+a2), r2 = a3/((double)a3+a4);
                  cntrib((ipoint){a->ip.x + r1*(b->ip.x-a->ip.x), 
                                    a->ip.y + r1*(b->ip.y-a->ip.y)},
                         b->ip, 1, "one");
                  cntrib(d->ip, (ipoint){
                    c->ip.x + r2*(d->ip.x - c->ip.x), 
                      c->ip.y + r2*(d->ip.y - c->ip.y)}, 1, "two");
                  ++a->in; --c->in;}
              if(o) cross(&ipa[j], &ipa[j+1], &ipb[k], &ipb[k+1], a1, a2, a3, a4);
              else cross(&ipb[k], &ipb[k+1], &ipa[j], &ipa[j+1], a3, a4, a1, a2);
            }
          }
        }
      }

    {
      void inness(vertex * P, int cP, vertex * Q, int cQ)
        {
          int s=0, c=cQ; ipoint p = P[0].ip;
          while(c--)if(Q[c].rx.mn < p.x && p.x < Q[c].rx.mx)
          {
            int sgn = 0 < area(p, Q[c].ip, Q[c+1].ip);
            s += sgn != Q[c].ip.x < Q[c+1].ip.x ? 0 : (sgn?-1:1);
          }
          {
            int j; for(j=0; j<cP; ++j)
            {
              if(s) cntrib(P[j].ip, P[j+1].ip, s, "three");
              s += P[j].in;
            }
          }
        }
      inness(ipa, na, ipb, nb); inness(ipb, nb, ipa, na);
    }
    return s/ascale;
  }
  
}

