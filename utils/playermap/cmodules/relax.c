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
 * Desc: Occupancy relax
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "gsl/gsl_blas.h"
#include "gsl/gsl_multifit_nlin.h"
#include "gsl/gsl_multimin.h"

#include "relax.h"

#define EPSILON 1e-16


// Least-squares optimization routines
static void relax_ls_link(relax_t *self, relax_link_t *link, vector_t pose_a, vector_t pose_b,
                          double *err, vector_t *grad_a, vector_t *grad_b);
static int relax_ls_fdf(const gsl_vector *x, relax_t *self, gsl_vector *f, gsl_matrix *J);
static int relax_ls_f(const gsl_vector *x, void *self, gsl_vector *f);
static int relax_ls_df(const gsl_vector *x, void *self, gsl_matrix *J);

// Non-linear fitting
static void relax_nl_link(relax_t *self, relax_link_t *link, vector_t pose_a, vector_t pose_b,
                          double *err, vector_t *grad_a, vector_t *grad_b);
static void relax_nl_fdf(const gsl_vector *x, relax_t *self, double *f, gsl_vector *df);
static double relax_nl_f(const gsl_vector *x, relax_t *self);
static void relax_nl_df(const gsl_vector *x, relax_t *self, gsl_vector *df);


// Create a new relaxation engine
relax_t *relax_alloc()
{
  relax_t *self;

  self = (relax_t*) malloc(sizeof(relax_t));

  self->node_max_count = 0;
  self->node_count = 0;
  self->nodes = NULL;

  self->link_max_count = 0;
  self->link_count = 0;
  self->links = NULL;
  
  return self;
}


// Destroy a relaxation engine
void relax_free(relax_t *self)
{  
  assert(self->link_count == 0);
  free(self->links);

  assert(self->node_count == 0);
  free(self->nodes);
      
  free(self);
  return;
}


// Add a node to the graph
relax_node_t *relax_node_alloc(relax_t *self)
{
  relax_node_t *node;

  if (self->node_count >= self->node_max_count)
  {
    self->node_max_count += 100;
    self->nodes = (relax_node_t**) realloc(self->nodes,
                                           self->node_max_count * sizeof(relax_node_t*));
  }

  node = calloc(1, sizeof(relax_node_t));
  self->nodes[self->node_count] = node;
  self->node_count++;
  
  return node;
}


// Remove a node from the graph
void relax_node_free(relax_t *self, relax_node_t *node)
{
  int i;
        
  for (i = 0; i < self->node_count; i++)
  {
    if (self->nodes[i] == node)
    {
      memmove(self->nodes + i, self->nodes + i + 1,
              (self->node_count - i - 1) * sizeof(self->nodes[0]));
      self->node_count--;
      break;
    }
  }

  free(node);
  return;
}


// Add a link to the graph
relax_link_t *relax_link_alloc(relax_t *self, relax_node_t *node_a, relax_node_t *node_b)
{
  relax_link_t *link;

  if (self->link_count >= self->link_max_count)
  {
    self->link_max_count += 100;
    self->links = (relax_link_t**) realloc(self->links,
                                           self->link_max_count * sizeof(relax_link_t*));
  }

  link = calloc(1, sizeof(relax_link_t));
  link->node_a = node_a;
  link->node_b = node_b;
  self->links[self->link_count] = link;
  self->link_count++;
  
  return link;
}


// Remove a link from the graph
void relax_link_free(relax_t *self, relax_link_t *link)
{
  int i;
        
  for (i = 0; i < self->link_count; i++)
  {
    if (self->links[i] == link)
    {
      memmove(self->links + i, self->links + i + 1,
              (self->link_count - i - 1) * sizeof(self->links[0]));
      self->link_count--;
      break;
    }
  }

  free(link);
  return;
}


/**************************************************************************
 * Levenberg-Marquardt least-squares
 **************************************************************************/

// Relax the graph
double relax_relax_ls(relax_t *self, int steps, double epsabs, double epsrel)
{
  int i, n, p;
  relax_node_t *node;
  relax_link_t *link;
  double err;
  const gsl_multifit_fdfsolver_type *type;
  gsl_vector *x;
  gsl_multifit_function_fdf func;
  gsl_multifit_fdfsolver *solver;
  
  // Count the number of functions
  n = self->link_count;

  // Count the number of free parameters
  p = 0;
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes[i];
    if (node->free)
    {
      node->index = p;
      p += 3;
    }
  }

  //printf("p = %d n = %d\n", p, n);
  
  // Solver segfaults unless n > p
  //assert(n >= p);
  if (n < p)
    return 0.0;
  
  // Construct description of function to be minimized.
  func.f = &relax_ls_f;
  func.df = &relax_ls_df;
  func.fdf = (void*) &relax_ls_fdf;
  func.n = n;
  func.p = p;
  func.params = self;

  // Create the solver
  type = gsl_multifit_fdfsolver_lmsder;
  solver = gsl_multifit_fdfsolver_alloc(type, n, p);

  // Construct vector describing the initial guess.
  x = gsl_vector_alloc(p);
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes[i];
    if (node->free)
    {
      gsl_vector_set(x, node->index + 0, node->pose.v[0]);
      gsl_vector_set(x, node->index + 1, node->pose.v[1]);
      gsl_vector_set(x, node->index + 2, node->pose.v[2]);
    }
  }

  // Initialize the solver
  gsl_multifit_fdfsolver_set(solver, &func, x);
  gsl_vector_free(x);
  
  // Iterate the solvers
  for (i = 0; i < steps; i++)
  {
    if (gsl_multifit_fdfsolver_iterate(solver) != 0)
      break;
    if (gsl_multifit_test_delta(solver->dx, solver->x, epsabs, epsrel) != GSL_CONTINUE)
      break;
    err = gsl_blas_dnrm2(solver->f);
    err = err * err;
    //printf("solver %f\n", err);
  }
  //printf("Steps %d\n", i);

  // Get the improved fit
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes[i];
    if (node->free)
    {
      node->pose.v[0] = gsl_vector_get(solver->x, node->index + 0);
      node->pose.v[1] = gsl_vector_get(solver->x, node->index + 1);
      node->pose.v[2] = gsl_vector_get(solver->x, node->index + 2);
      assert(finite(node->pose.v[0]));
      assert(finite(node->pose.v[1]));
      assert(finite(node->pose.v[2]));
    }
  }

  // Compute the total error
  err = 0;
  for (i = 0; i < self->link_count; i++)
  {
    link = self->links[i];
    assert(link->node_a->free || link->node_b->free);
    err += gsl_vector_get(solver->f, i) * gsl_vector_get(solver->f, i);
  }

  // Destroy the solver
  gsl_multifit_fdfsolver_free(solver);

  return err;
}


// Compute error and gradient terms
int relax_ls_fdf(const gsl_vector *x, relax_t *self, gsl_vector *f, gsl_matrix *J)
{
  int l;
  double err;
  relax_link_t *link;
  relax_node_t *node_a, *node_b;
  vector_t pose_a, pose_b;
  vector_t grad_a, grad_b;
    
  for (l = 0; l < self->link_count; l++)
  {
    link = self->links[l];
    node_a = link->node_a;
    node_b = link->node_b;

    if (!node_a->free && !node_b->free)
      continue;

    if (node_a->free)
    {
      pose_a.v[0] = gsl_vector_get(x, node_a->index + 0);
      pose_a.v[1] = gsl_vector_get(x, node_a->index + 1);
      pose_a.v[2] = gsl_vector_get(x, node_a->index + 2);
    }
    else
      pose_a = node_a->pose;

    if (node_b->free)
    {
      pose_b.v[0] = gsl_vector_get(x, node_b->index + 0);
      pose_b.v[1] = gsl_vector_get(x, node_b->index + 1);
      pose_b.v[2] = gsl_vector_get(x, node_b->index + 2);
    }
    else
      pose_b = node_b->pose;

    if (node_a->free && node_b->free)
      relax_ls_link(self, link, pose_a, pose_b, &err, &grad_a, &grad_b);
    else if (node_a->free && !node_b->free)
      relax_ls_link(self, link, pose_a, pose_b, &err, &grad_a, NULL);
    else if (!node_a->free && node_b->free)
      relax_ls_link(self, link, pose_a, pose_b, &err, NULL, &grad_b);

    if (f)
    {
      gsl_vector_set(f, l, err);
    }

    if (J)
    {
      if (node_a->free)
      {
        gsl_matrix_set(J, l, node_a->index + 0, grad_a.v[0]);
        gsl_matrix_set(J, l, node_a->index + 1, grad_a.v[1]);
        gsl_matrix_set(J, l, node_a->index + 2, grad_a.v[2]);
      }

      if (node_b->free)
      {
        gsl_matrix_set(J, l, node_b->index + 0, grad_b.v[0]);
        gsl_matrix_set(J, l, node_b->index + 1, grad_b.v[1]);
        gsl_matrix_set(J, l, node_b->index + 2, grad_b.v[2]);
      }
    }
  }

  return GSL_SUCCESS;
}


// Compute the error
int relax_ls_f(const gsl_vector *x, void *self, gsl_vector *f)
{
  return relax_ls_fdf(x, self, f, NULL);
}


// Compute the gradient
int relax_ls_df(const gsl_vector *x, void *self, gsl_matrix *J)
{
  return relax_ls_fdf(x, self, NULL, J);
}


// Compute the error and gradient terms for a link
void relax_ls_link(relax_t *self, relax_link_t *link, vector_t pose_a, vector_t pose_b,
                   double *err, vector_t *grad_a, vector_t *grad_b)
{
  double w, max_s;
  double cos_a, sin_a, cos_b, sin_b;
  geom_point_t p, pp;
  geom_point_t q, qq;
  geom_line_t l, ll;
  double u, s, du_ds;
  double ds_dp[2], ds_dq[2];
  double du_da[3], dp_da[2][3];
  double du_db[3], dq_db[2][3];
  
  cos_a = cos(pose_a.v[2]);
  sin_a = sin(pose_a.v[2]);

  cos_b = cos(pose_b.v[2]);
  sin_b = sin(pose_b.v[2]);

  w = link->w;
  max_s = link->outlier;
    
  // Point/point link
  if (link->type == 0)
  {
    pp = link->pa;
    qq = link->pb;

    // Compute the points in the global cs
    p.x = pose_a.v[0] + pp.x * cos_a - pp.y * sin_a;
    p.y = pose_a.v[1] + pp.x * sin_a + pp.y * cos_a;

    q.x = pose_b.v[0] + qq.x * cos_b - qq.y * sin_b;
    q.y = pose_b.v[1] + qq.x * sin_b + qq.y * cos_b;

    // Distance between the points
    s = sqrt((q.x - p.x) * (q.x - p.x) + (q.y - p.y) * (q.y - p.y));
  }

  // Point/line link
  else if (link->type == 1)
  {
    pp = link->pa;
    ll = link->lb;

    // Compute the points in the global cs
    p.x = pose_a.v[0] + pp.x * cos_a - pp.y * sin_a;
    p.y = pose_a.v[1] + pp.x * sin_a + pp.y * cos_a;

    l.pa.x = pose_b.v[0] + ll.pa.x * cos_b - ll.pa.y * sin_b;
    l.pa.y = pose_b.v[1] + ll.pa.x * sin_b + ll.pa.y * cos_b;
    l.pb.x = pose_b.v[0] + ll.pb.x * cos_b - ll.pb.y * sin_b;
    l.pb.y = pose_b.v[1] + ll.pb.x * sin_b + ll.pb.y * cos_b;

    // Find the nearest point on the line
    s = geom_line_nearest(&l, &p, &q);
  
    // Compute the nearest point relative to b
    qq.x = +(q.x - pose_b.v[0]) * cos_b + (q.y - pose_b.v[1]) * sin_b;
    qq.y = -(q.x - pose_b.v[0]) * sin_b + (q.y - pose_b.v[1]) * cos_b;
  }

  // Line/point link
  else if (link->type == 2)
  {
    ll = link->la;
    qq = link->pb;

    // Compute the points in the global cs
    l.pa.x = pose_a.v[0] + ll.pa.x * cos_a - ll.pa.y * sin_a;
    l.pa.y = pose_a.v[1] + ll.pa.x * sin_a + ll.pa.y * cos_a;
    l.pb.x = pose_a.v[0] + ll.pb.x * cos_a - ll.pb.y * sin_a;
    l.pb.y = pose_a.v[1] + ll.pb.x * sin_a + ll.pb.y * cos_a;

    q.x = pose_b.v[0] + qq.x * cos_b - qq.y * sin_b;
    q.y = pose_b.v[1] + qq.x * sin_b + qq.y * cos_b;

    // Find the nearest point on the line
    s = geom_line_nearest(&l, &q, &p);
  
    // Compute the nearest point relative to a
    pp.x = +(p.x - pose_a.v[0]) * cos_a + (p.y - pose_a.v[1]) * sin_a;
    pp.y = -(p.x - pose_a.v[0]) * sin_a + (p.y - pose_a.v[1]) * cos_a;
  }
  else
    assert(0);

  // Compute the error term
  if (s < max_s)
  {
    u = w * s;
    du_ds = w;
  }
  else
  {
    u = w * max_s;
    du_ds = 0.0;
  }
  
  *err = u;

  if (grad_a != NULL)
  {
    if (s > EPSILON)
    {
      ds_dp[0] = -(q.x - p.x) / s;
      ds_dp[1] = -(q.y - p.y) / s;
    }
    else
    {
      ds_dp[0] = 0;
      ds_dp[1] = 0;
    }

    dp_da[0][0] = 1;
    dp_da[0][1] = 0;
    dp_da[0][2] = -pp.x * sin_a - pp.y * cos_a;

    dp_da[1][0] = 0;
    dp_da[1][1] = 1;
    dp_da[1][2] = +pp.x * cos_a - pp.y * sin_a;

    du_da[0] = du_ds * (ds_dp[0] * dp_da[0][0] + ds_dp[1] * dp_da[1][0]);
    du_da[1] = du_ds * (ds_dp[0] * dp_da[0][1] + ds_dp[1] * dp_da[1][1]);
    du_da[2] = du_ds * (ds_dp[0] * dp_da[0][2] + ds_dp[1] * dp_da[1][2]);

    grad_a->v[0] = du_da[0];
    grad_a->v[1] = du_da[1];
    grad_a->v[2] = du_da[2];
  }

  if (grad_b != NULL)
  {
    if (s > EPSILON)
    {
      ds_dq[0] = +(q.x - p.x) / s;
      ds_dq[1] = +(q.y - p.y) / s;
    }
    else
    {
      ds_dq[0] = 0;
      ds_dq[1] = 0;
    }

    dq_db[0][0] = 1;
    dq_db[0][1] = 0;
    dq_db[0][2] = -qq.x * sin_b - qq.y * cos_b;
  
    dq_db[1][0] = 0;
    dq_db[1][1] = 1;
    dq_db[1][2] = +qq.x * cos_b - qq.y * sin_b;

    du_db[0] = du_ds * (ds_dq[0] * dq_db[0][0] + ds_dq[1] * dq_db[1][0]);
    du_db[1] = du_ds * (ds_dq[0] * dq_db[0][1] + ds_dq[1] * dq_db[1][1]);
    du_db[2] = du_ds * (ds_dq[0] * dq_db[0][2] + ds_dq[1] * dq_db[1][2]);
  
    grad_b->v[0] = du_db[0];
    grad_b->v[1] = du_db[1];
    grad_b->v[2] = du_db[2];
  }
  
  return;
}




/**************************************************************************
 * Non-linear optimization
 **************************************************************************/

// Relax the graph
double relax_relax_nl(relax_t *self, int steps, double epsabs, double step, double tol)
{
  int i, p;
  double err, lasterr;
  relax_node_t *node;
  const gsl_multimin_fdfminimizer_type *type;
  gsl_multimin_function_fdf func;
  gsl_multimin_fdfminimizer *solver;
  gsl_vector *x;

  // Count the number of free parameters
  p = 0;
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes[i];
    if (node->free)
    {
      node->index = p;
      p += 3;
    }
  }
  
  // Create the minimizer
  type = gsl_multimin_fdfminimizer_conjugate_fr;
  //type = gsl_multimin_fdfminimizer_steepest_descent;
  solver = gsl_multimin_fdfminimizer_alloc(type, p);

  // Construct description of function to be minimized.
  func.f = (void*) &relax_nl_f;
  func.df = (void*) &relax_nl_df;
  func.fdf = (void*) &relax_nl_fdf;
  func.n = p;
  func.params = self;

  // Construct vector describing the initial guess.
  x = gsl_vector_alloc(p);
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes[i];
    if (node->free)
    {
      gsl_vector_set(x, node->index + 0, node->pose.v[0]);
      gsl_vector_set(x, node->index + 1, node->pose.v[1]);
      gsl_vector_set(x, node->index + 2, node->pose.v[2]);
    }
  }
  
  // Set the initial guess.
  gsl_multimin_fdfminimizer_set(solver, &func, x, step, tol);
  gsl_vector_free(x);

  // TESTING
  //printf("n = %d l = %d\n", p, self->link_count);
  
  // Iterate the solver
  err = 0.0;
  lasterr = DBL_MAX / 2;
  for (i = 0; i < steps; i++)
  {
    if (gsl_multimin_fdfminimizer_iterate(solver) != 0)
      break;

    err = gsl_multimin_fdfminimizer_minimum(solver);

    //if (fabs(err - lasterr) < epsabs)
    // break;
    if (fabs(err - lasterr) / (lasterr + 1e-16) < epsabs)
     break;

    lasterr = err;
  }

  //printf("err = %d %f\n", i, err);

  // Get the improved fit
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes[i];
    if (node->free)
    {
      node->pose.v[0] = gsl_vector_get(solver->x, node->index + 0);
      node->pose.v[1] = gsl_vector_get(solver->x, node->index + 1);
      node->pose.v[2] = gsl_vector_get(solver->x, node->index + 2);
    }
  }

  err = gsl_multimin_fdfminimizer_minimum(solver);
  
  // Destroy the solver
  gsl_multimin_fdfminimizer_free(solver);
  
  return err;
}


// Compute the error and gradient terms for a link
void relax_nl_link(relax_t *self, relax_link_t *link, vector_t pose_a, vector_t pose_b,
                   double *err, vector_t *grad_a, vector_t *grad_b)
{
  double w, max_s;
  double cos_a, sin_a, cos_b, sin_b;
  geom_point_t p, pp;
  geom_point_t q, qq;
  geom_line_t l, ll;
  double u, s, du_ds;
  double ds_dp[2], ds_dq[2];
  double du_da[3], dp_da[2][3];
  double du_db[3], dq_db[2][3];

  cos_a = cos(pose_a.v[2]);
  sin_a = sin(pose_a.v[2]);

  cos_b = cos(pose_b.v[2]);
  sin_b = sin(pose_b.v[2]);

  w = link->w;
  max_s = link->outlier;
  
  // Point/point link
  if (link->type == 0)
  {
    pp = link->pa;
    qq = link->pb;

    // Compute the points in the global cs
    p.x = pose_a.v[0] + pp.x * cos_a - pp.y * sin_a;
    p.y = pose_a.v[1] + pp.x * sin_a + pp.y * cos_a;

    q.x = pose_b.v[0] + qq.x * cos_b - qq.y * sin_b;
    q.y = pose_b.v[1] + qq.x * sin_b + qq.y * cos_b;

    // Distance between the points
    s = sqrt((q.x - p.x) * (q.x - p.x) + (q.y - p.y) * (q.y - p.y));
  }

  // Point/line link
  else if (link->type == 1)
  {
    pp = link->pa;
    ll = link->lb;

    // Compute the points in the global cs
    p.x = pose_a.v[0] + pp.x * cos_a - pp.y * sin_a;
    p.y = pose_a.v[1] + pp.x * sin_a + pp.y * cos_a;

    l.pa.x = pose_b.v[0] + ll.pa.x * cos_b - ll.pa.y * sin_b;
    l.pa.y = pose_b.v[1] + ll.pa.x * sin_b + ll.pa.y * cos_b;
    l.pb.x = pose_b.v[0] + ll.pb.x * cos_b - ll.pb.y * sin_b;
    l.pb.y = pose_b.v[1] + ll.pb.x * sin_b + ll.pb.y * cos_b;

    // Find the nearest point on the line
    s = geom_line_nearest(&l, &p, &q);
  
    // Compute the nearest point relative to b
    qq.x = +(q.x - pose_b.v[0]) * cos_b + (q.y - pose_b.v[1]) * sin_b;
    qq.y = -(q.x - pose_b.v[0]) * sin_b + (q.y - pose_b.v[1]) * cos_b;
  }

  // Line/point link
  else if (link->type == 2)
  {
    ll = link->la;
    qq = link->pb;

    // Compute the points in the global cs
    l.pa.x = pose_a.v[0] + ll.pa.x * cos_a - ll.pa.y * sin_a;
    l.pa.y = pose_a.v[1] + ll.pa.x * sin_a + ll.pa.y * cos_a;
    l.pb.x = pose_a.v[0] + ll.pb.x * cos_a - ll.pb.y * sin_a;
    l.pb.y = pose_a.v[1] + ll.pb.x * sin_a + ll.pb.y * cos_a;

    q.x = pose_b.v[0] + qq.x * cos_b - qq.y * sin_b;
    q.y = pose_b.v[1] + qq.x * sin_b + qq.y * cos_b;

    // Find the nearest point on the line
    s = geom_line_nearest(&l, &q, &p);
  
    // Compute the nearest point relative to a
    pp.x = +(p.x - pose_a.v[0]) * cos_a + (p.y - pose_a.v[1]) * sin_a;
    pp.y = -(p.x - pose_a.v[0]) * sin_a + (p.y - pose_a.v[1]) * cos_a;
  }
  else
    assert(0);

  // Compute the error term
  if (s < max_s) // HACK
  {
    u = 0.5 * w * s * s;
    du_ds = w * s;
  }
  else
  {
    u = 0.5 * w * max_s * max_s;
    du_ds = 0;
  }
  
  *err = u;

  if (s > EPSILON)
  {
    ds_dp[0] = -(q.x - p.x) / s;
    ds_dp[1] = -(q.y - p.y) / s;
  }
  else
  {
    ds_dp[0] = 0;
    ds_dp[1] = 0;
  }

  dp_da[0][0] = 1;
  dp_da[0][1] = 0;
  dp_da[0][2] = -pp.x * sin_a - pp.y * cos_a;

  dp_da[1][0] = 0;
  dp_da[1][1] = 1;
  dp_da[1][2] = +pp.x * cos_a - pp.y * sin_a;

  du_da[0] = du_ds * (ds_dp[0] * dp_da[0][0] + ds_dp[1] * dp_da[1][0]);
  du_da[1] = du_ds * (ds_dp[0] * dp_da[0][1] + ds_dp[1] * dp_da[1][1]);
  du_da[2] = du_ds * (ds_dp[0] * dp_da[0][2] + ds_dp[1] * dp_da[1][2]);

  grad_a->v[0] = du_da[0];
  grad_a->v[1] = du_da[1];
  grad_a->v[2] = du_da[2];

  if (s > EPSILON)
  {
    ds_dq[0] = +(q.x - p.x) / s;
    ds_dq[1] = +(q.y - p.y) / s;
  }
  else
  {
    ds_dq[0] = 0;
    ds_dq[1] = 0;
  }

  dq_db[0][0] = 1;
  dq_db[0][1] = 0;
  dq_db[0][2] = -qq.x * sin_b - qq.y * cos_b;
  
  dq_db[1][0] = 0;
  dq_db[1][1] = 1;
  dq_db[1][2] = +qq.x * cos_b - qq.y * sin_b;

  du_db[0] = du_ds * (ds_dq[0] * dq_db[0][0] + ds_dq[1] * dq_db[1][0]);
  du_db[1] = du_ds * (ds_dq[0] * dq_db[0][1] + ds_dq[1] * dq_db[1][1]);
  du_db[2] = du_ds * (ds_dq[0] * dq_db[0][2] + ds_dq[1] * dq_db[1][2]);
  
  grad_b->v[0] = du_db[0];
  grad_b->v[1] = du_db[1];
  grad_b->v[2] = du_db[2];
  
  return;
}


// Compute both error and gradient.
void relax_nl_fdf(const gsl_vector *x, relax_t *self, double *f, gsl_vector *df)
{
  int l;
  double err;
  relax_link_t *link;
  relax_node_t *node_a, *node_b;
  vector_t pose_a, pose_b;
  vector_t grad_a, grad_b;

  if (f)
    *f = 0;
  if (df)
    gsl_vector_set_zero(df);

  for (l = 0; l < self->link_count; l++)
  {
    link = self->links[l];
    node_a = link->node_a;
    node_b = link->node_b;

    if (!node_a->free && !node_b->free)
      continue;

    if (node_a->free)
    {
      pose_a.v[0] = gsl_vector_get(x, node_a->index + 0);
      pose_a.v[1] = gsl_vector_get(x, node_a->index + 1);
      pose_a.v[2] = gsl_vector_get(x, node_a->index + 2);
    }
    else
      pose_a = node_a->pose;

    if (node_b->free)
    {
      pose_b.v[0] = gsl_vector_get(x, node_b->index + 0);
      pose_b.v[1] = gsl_vector_get(x, node_b->index + 1);
      pose_b.v[2] = gsl_vector_get(x, node_b->index + 2);
    }
    else
      pose_b = node_b->pose;

    relax_nl_link(self, link, pose_a, pose_b, &err, &grad_a, &grad_b);

    if (f)
    {
      *f += err;
    }

    if (df)
    {
      if (node_a->free)
      {
        df->data[node_a->index + 0] += grad_a.v[0];
        df->data[node_a->index + 1] += grad_a.v[1];
        df->data[node_a->index + 2] += grad_a.v[2];
      }

      if (node_b->free)
      {
        df->data[node_b->index + 0] += grad_b.v[0];
        df->data[node_b->index + 1] += grad_b.v[1];
        df->data[node_b->index + 2] += grad_b.v[2];
      }
    }
  }

  return;
}



// Compute the error 
double relax_nl_f(const gsl_vector *x, relax_t *self)
{
  double f;
  relax_nl_fdf(x, self, &f, NULL);
  return f;
}


// Compute the gradient
void relax_nl_df(const gsl_vector *x, relax_t *self, gsl_vector *df)
{
  relax_nl_fdf(x, self, NULL, df);
  return;
}





