
/**************************************************************************
 * Desc: Local map functions for fitting grids
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <gsl/gsl_errno.h>
#include <gsl/gsl_multimin.h>

#include "imap.h"


// Pre-computed cell data for fitting
typedef struct
{
  // Cell pointer
  imap_cell_t *cell;
  
  // Cell position
  double cx, cy;

} imap_fit_cell_t;


// Structure for map/map fitting
typedef struct
{
  // Map a
  imap_t *imap_a;
  int imap_a_cell_count;
  imap_fit_cell_t *imap_a_cells;

  // Map b
  imap_t *imap_b;
  int imap_b_cell_count;
  imap_fit_cell_t *imap_b_cells;

  // Initial pose of map_b relative to map_a
  double map_pose[3];
  
} imap_fit_t;

// Compute the best fit pose between two maps.
double imap_fit(imap_t *imap_a, imap_t *imap_b, double robot_pose[3]);

// Create the fit data; involves some pre-processing of the grid to
// save time during optimization.
imap_fit_t *imap_fit_alloc(imap_t *imap_a, imap_t *imap_b, double robot_pose[3]);

// Free the fit data
void imap_fit_free(imap_fit_t *fit);

// Compute the fit error (gsl callback)
double imap_fit_func(double x, imap_fit_t *fit);

// Compute the fit error (gsl callback)
double imap_fit_f(const gsl_vector *x, imap_fit_t *fit);

// Compute the gradient of the error (gsl callback)
void imap_fit_df(const gsl_vector *x, imap_fit_t *fit, gsl_vector *df);

// Compute both the error and the gradient of the error
void imap_fit_fdf(const gsl_vector *x, imap_fit_t *fit, double *f, gsl_vector *df);

// Compute the disparity between two imaps (gsl callback)
double imap_fit_compare(imap_fit_t *fit, double pose[3], double grad[3]);

// Normalize angles
#define NORMALIZE(a) atan2(sin(a), cos(a))


// Compute the best fit pose between a range scan and the map
double imap_fit_ranges(imap_t *imap, double robot_pose[3], double laser_pose[3],
                       int range_count, double ranges[][2])
{
  double err;
  double zero_pose[3];
  imap_t *imap_laser;

  // HACK: size
  // Create a new map
  imap_laser = imap_alloc(16.0 / imap->scale, 16.0 / imap->scale, imap->scale,
                          imap->max_fit_dist, imap->max_fit_dist);

  // Set the sensor model parameters so that cells
  // get assigned immediately to occupied or empty.
  imap_laser->model_occ_thresh = imap_laser->model_occ_inc;
  imap_laser->model_emp_thresh = imap_laser->model_emp_inc;
  
  // Add the laser scan to the new map
  zero_pose[0] = 0.0;
  zero_pose[1] = 0.0;
  zero_pose[2] = 0.0;
  imap_add_ranges(imap_laser, zero_pose, laser_pose, range_count, ranges);
  
  // Find the best fit between the new map and the old map
  err = imap_fit(imap, imap_laser, robot_pose);

  // Free the temp map; we're done with it
  imap_free(imap_laser);
  
  return err;
}


// Compute the best fit pose between two maps.
double imap_fit(imap_t *imap_a, imap_t *imap_b, double map_pose[3])
{
  int i, status;
  double err;
  imap_fit_t *fit;
  double x, xlower, xupper;
  double e, elower, eupper;
  gsl_function f;
  gsl_min_fminimizer *s;
  
  // Initialize fit data
  fit = imap_fit_alloc(imap_a, imap_b, map_pose);

  // Create a minimizer
  s = gsl_min_fminimizer_alloc(gsl_min_fminimizer_brent);
  assert(s != NULL);
  
  // Set the function to minimize
  f.function = imap_fit_func;
  f.params = fit;

  // Initial guess
  x = map_pose[2];
  xlower = x - 30 * M_PI / 180;
  xupper = x + 30 * M_PI / 180;

  /*
  // TESTING
  for (i = -50; i <= 50; i++)
  {
    printf("%d %f %f\n", i,
           x + i * (xupper - xlower) / 100,
           imap_fit_func(x + i * (xupper - xlower) / 100, fit));
  }
  printf("\n\n");
  */
  
  e = imap_fit_func(x, fit);
  elower = imap_fit_func(xlower, fit);
  eupper = imap_fit_func(xupper, fit);

  // Must enclose a minimum to work
  if (e >= elower || e >= eupper)
  {
    //printf("%f %f %f\n", elower, e, eupper);
    //printf("0 0 0\n");
    //printf("\n\n");
    return e;
  }

  // Initialize minimizer
  status = gsl_min_fminimizer_set_with_values(s, &f, x, e, xlower, elower, xupper, eupper);
  assert(status == 0);

  // Do the fitting
  for (i = 0; i < 20; i++)
  {
    status = gsl_min_fminimizer_iterate(s);
    if (status == GSL_FAILURE)
      break;
    if (status != 0)
    {
      fprintf(stderr, "gsl error: %s\n", gsl_strerror(status));
      break;
    }

    // TESTING
    //err = gsl_min_fminimizer_f_minimum(s);
    //x = gsl_min_fminimizer_x_minimum(s);
    //printf("%d %f %f\n", i, x, err);
  }
  //printf("\n\n");
  
  err = gsl_min_fminimizer_f_minimum(s);
  map_pose[2] = gsl_min_fminimizer_x_minimum(s);

  gsl_min_fminimizer_free(s);
  imap_fit_free(fit);
  
  return err;
}


// Compute the best fit pose between two imaps.
double imap_fit_x(imap_t *imap_a, imap_t *imap_b, double map_pose[3])
{
  int i, status;
  double err;
  imap_fit_t *fit;
  gsl_vector *x;
  gsl_multimin_function_fdf f;
  gsl_multimin_fdfminimizer *s;

  // TESTING
  double pose[3], grad[3];
  
  // Initialize fit data
  fit = imap_fit_alloc(imap_a, imap_b, map_pose);

  /*
  // TESTING
  for (i = -50; i <= 50; i++)
  {
    pose[0] = *ox;
    pose[1] = *oy;
    pose[2] = *oa + i * 0.1 * M_PI / 180;

    err = imap_fit_compare(fit, pose, grad);

    printf("%d %f %f %f %f %f %f %f\n", i, err,
           pose[0], pose[1], pose[2],
           grad[0], grad[1], grad[2]);
  }
  printf("\n\n");
  */

  // Create a minimizer
  s = gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_conjugate_fr, 3);
  //s = gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_vector_bfgs, 3);
  //s = gsl_multimin_fdfminimizer_alloc(gsl_multimin_fdfminimizer_steepest_descent, 3);
  assert(s != NULL);
  
  // Set the function to minimize
  f.f = imap_fit_f;
  f.df = imap_fit_df;
  f.fdf = imap_fit_fdf;
  f.n = 3;
  f.params = fit;

  // Initial guess
  x = gsl_vector_alloc(3);
  gsl_vector_set(x, 0, map_pose[0]);
  gsl_vector_set(x, 1, map_pose[1]);
  gsl_vector_set(x, 2, map_pose[2]);

  // Initialize minimizer
  status = gsl_multimin_fdfminimizer_set(s, &f, x, 1e-3, 1e-6);
  assert(status == 0);
  
  // TESTING
  //printf("%d %f %f %f %f\n", 0, s->f,
  //       gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1), gsl_vector_get(s->x, 2));

  // Do the fitting
  for (i = 0; i < 5; i++)
  {
    // TESTING
    //printf("%d %f %f %f %f\n", i, s->f,
    //       gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1), gsl_vector_get(s->x, 2));

    status = gsl_multimin_fdfminimizer_iterate(s);
    if (status == GSL_ENOPROG)
      break;
    if (status != 0)
    {
      fprintf(stderr, "gsl error: %s\n", gsl_strerror(status));
      break;
    }

    // HACK
    status = gsl_multimin_test_gradient(s->gradient, 1e-3);
    if (status == GSL_SUCCESS)
      break;
  }

  // TESTING
  //printf("%d %f %f %f %f\n", i, s->f,
  //       gsl_vector_get(s->x, 0), gsl_vector_get(s->x, 1), gsl_vector_get(s->x, 2));
  //printf("\n\n");

  err = s->f;
  map_pose[0] = gsl_vector_get(s->x, 0);
  map_pose[1] = gsl_vector_get(s->x, 1);
  map_pose[2] = gsl_vector_get(s->x, 2);

  gsl_multimin_fdfminimizer_free(s);
  gsl_vector_free(x);
  imap_fit_free(fit);
  
  return err;
}


// Create the fit data; involves some pre-processing of the grid to
// save time during optimization.
imap_fit_t *imap_fit_alloc(imap_t *imap_a, imap_t *imap_b, double map_pose[3])
{
  int i, j;
  imap_fit_t *fit;
  imap_fit_cell_t *fcell;
  imap_cell_t *cell;

  fit = malloc(sizeof(imap_fit_t));

  fit->imap_a = imap_a;
  fit->imap_b = imap_b;
  
  fit->map_pose[0] = map_pose[0];
  fit->map_pose[1] = map_pose[1];
  fit->map_pose[2] = map_pose[2];

  // Allocate the max possible space for occupied cells
  fit->imap_a_cells = malloc(imap_a->size_x * imap_a->size_y * sizeof(imap_fit_cell_t));
  assert(fit->imap_a_cells);
  fit->imap_a_cell_count = 0;
  
  // Prepare the list of occupied cells in map_a
  for (j = 0; j < imap_a->size_y; j++)
  {
    for (i = 0; i < imap_a->size_x; i++)
    {
      cell = imap_a->cells + IMAP_INDEX(imap_a, i, j);
      if (cell->occ_state == +1)
      {
        fcell = fit->imap_a_cells + fit->imap_a_cell_count++;
        fcell->cx = IMAP_WXGX(imap_a, i);
        fcell->cy = IMAP_WYGY(imap_a, j);
        fcell->cell = cell;
      }
    }
  }

  // Allocate the max possible space for occupied cells
  fit->imap_b_cells = malloc(imap_b->size_x * imap_b->size_y * sizeof(imap_fit_cell_t));
  assert(fit->imap_b_cells);
  fit->imap_b_cell_count = 0;

  // Prepare the list of occupied cells in map_b
  for (j = 0; j < imap_b->size_y; j++)
  {
    for (i = 0; i < imap_b->size_x; i++)
    {
      cell = imap_b->cells + IMAP_INDEX(imap_b, i, j);
      if (cell->occ_state == +1)
      {
        fcell = fit->imap_b_cells + fit->imap_b_cell_count++;
        fcell->cx = IMAP_WXGX(imap_b, i);
        fcell->cy = IMAP_WYGY(imap_b, j);
        fcell->cell = cell;
      }
    }
  }

  return fit;
}


// Free the fit data
void imap_fit_free(imap_fit_t *fit)
{
  free(fit->imap_b_cells);  
  free(fit->imap_a_cells);
  free(fit);
  return;
}


// Compute the error
double imap_fit_func(double x, imap_fit_t *fit)
{
  double pose[3];
  pose[0] = fit->map_pose[0];
  pose[1] = fit->map_pose[1];
  pose[2] = x;
  return imap_fit_compare(fit, pose, NULL);
}


// Compute the error
double imap_fit_f(const gsl_vector *x, imap_fit_t *fit)
{
  return imap_fit_compare(fit, x->data, NULL);
}


// Compute the gradient of the error
void imap_fit_df(const gsl_vector *x, imap_fit_t *fit, gsl_vector *df)
{
  imap_fit_compare(fit, x->data, df->data);
  return;
}


// Compute both the error and the gradient of the error
void imap_fit_fdf(const gsl_vector *x, imap_fit_t *fit, double *f, gsl_vector *df)
{  
  *f = imap_fit_compare(fit, x->data, df->data);
  return;
}


// Compute the disparity between two imaps.
double imap_fit_compare(imap_fit_t *fit, double pose[3], double grad[3])
{
  int i;
  int ai, aj, bi, bj;
  double ax, ay, bx, by;
  double nx, ny, dx, dy, da;
  double cb, sb;
  double k;
  double f, d;
  double u[3], um;
  double df[3];
  double dfda[2], dadp[2][3];
  double dfdb[2], dbdp[2][3];
  imap_cell_t *cell_a, *cell_b;
  imap_fit_cell_t *fcell;
  
  cb = cos(pose[2]);
  sb = sin(pose[2]);

  f = 0.0;
  
  df[0] = 0.0;
  df[1] = 0.0;
  df[2] = 0.0;

  // HACK
  k = 0;
  u[0] = 0.10;
  u[1] = 0.10;
  u[2] = 45 * M_PI / 180;
  um = 0.05;

  // Start with odometric estimate
  dx = (pose[0] - fit->map_pose[0]) / u[0];
  dy = (pose[1] - fit->map_pose[1]) / u[1];
  da = NORMALIZE(pose[2] - fit->map_pose[2]) / u[2];
  f += k * (dx * dx + dy * dy + da * da);

  if (grad)
  {
    df[0] += k * dx;
    df[1] += k * dy;
    df[2] += k * da;
  }
  
  // Iterate over the occupied cells in imap B
  for (i = 0; i < fit->imap_b_cell_count; i++)
  {
    fcell = fit->imap_b_cells + i;
    cell_b = fcell->cell;
    assert(cell_b->occ_state == +1);
    
    // Pose of cell b in map b's coordinate system
    nx = fcell->cx;
    ny = fcell->cy;

    // Pose of cell b in map a's coordinate system
    bx = pose[0] + nx * cb - ny * sb;
    by = pose[1] + nx * sb + ny * cb;

    // Cell index of cell b in map a
    bi = IMAP_GXWX(fit->imap_a, bx);
    bj = IMAP_GYWY(fit->imap_a, by);

    if (IMAP_VALID(fit->imap_a, bi, bj))
    {
      cell_a = fit->imap_a->cells + IMAP_INDEX(fit->imap_a, bi, bj);

      if (cell_a->occ_dist < fit->imap_a->max_fit_dist)
      {
        // Cell index of the nearest occupied cell in map a
        ai = bi + cell_a->occ_di;
        aj = bj + cell_a->occ_dj;

        // Pose of cell a in map a's coordinate system
        ax = IMAP_WXGX(fit->imap_a, ai);
        ay = IMAP_WYGY(fit->imap_a, aj);

        dx = (bx - ax) / um;
        dy = (by - ay) / um;

        d = sqrt(dx * dx + dy * dy);
        f += d * d;

        if (grad)
        {
          dfdb[0] = dx;
          dfdb[1] = dy;

          dbdp[0][0] = 1;
          dbdp[0][1] = 0;
          dbdp[0][2] = -nx * sb - ny * cb;

          dbdp[1][0] = 0;
          dbdp[1][1] = 1;
          dbdp[1][2] = +nx * cb - ny * sb;

          df[0] += dfdb[0] * dbdp[0][0] + dfdb[1] * dbdp[1][0];
          df[1] += dfdb[0] * dbdp[0][1] + dfdb[1] * dbdp[1][1];
          df[2] += dfdb[0] * dbdp[0][2] + dfdb[1] * dbdp[1][2];
        }
      }
      else
      {
        d = fit->imap_a->max_fit_dist / um;
        f += d * d;
      }
    }
    else
    {
      d = fit->imap_a->max_fit_dist / um;
      f += d * d;
    }
  }

  
  // Iterate over the occupied cells in imap A
  for (i = 0; i < fit->imap_a_cell_count; i++)
  {
    fcell = fit->imap_a_cells + i;
    cell_a = fcell->cell;
    assert(cell_a->occ_state == +1);
    
    // Pose of cell a in map a's coordinate system
    nx = fcell->cx;
    ny = fcell->cy;

    // Pose of cell a in map b's coordinate system
    ax = +(nx - pose[0]) * cb + (ny - pose[1]) * sb;
    ay = -(nx - pose[0]) * sb + (ny - pose[1]) * cb;

    // Cell index of cell a in map b
    ai = IMAP_GXWX(fit->imap_b, ax);
    aj = IMAP_GYWY(fit->imap_b, ay);
    
    if (IMAP_VALID(fit->imap_b, ai, aj))
    {
      cell_b = fit->imap_b->cells + IMAP_INDEX(fit->imap_b, ai, aj);

      if (cell_b->occ_dist < fit->imap_b->max_fit_dist)
      {
        // Cell index of the nearest occupied cell in map b
        bi = ai + cell_b->occ_di;
        bj = aj + cell_b->occ_dj;

        // Pose of cell b in map b's coordinate system
        bx = IMAP_WXGX(fit->imap_b, bi);
        by = IMAP_WYGY(fit->imap_b, bj);

        dx = (ax - bx) / um;
        dy = (ay - by) / um;

        d = sqrt(dx * dx + dy * dy);
        f += d * d;

        if (grad)
        {
          dfda[0] = dx;
          dfda[1] = dy;

          dadp[0][0] = -cb;
          dadp[0][1] = -sb;
          dadp[0][2] = -(nx - pose[0]) * sb + (ny - pose[1]) * cb; 

          dadp[1][0] = +sb;
          dadp[1][1] = -cb;
          dadp[1][2] = -(nx - pose[0]) * cb - (ny - pose[1]) * sb; 

          df[0] += dfda[0] * dadp[0][0] + dfda[1] * dadp[1][0];
          df[1] += dfda[0] * dadp[0][1] + dfda[1] * dadp[1][1];
          df[2] += dfda[0] * dadp[0][2] + dfda[1] * dadp[1][2];
        }
      }
      else
      {
        d = fit->imap_b->max_fit_dist / um;
        f += d * d;
      }
    }
    else
    {
      d = fit->imap_b->max_fit_dist / um;
      f += d * d;
    }
  }

  //printf("pose %f %f %f\n", pose[0], pose[1], pose[2]);
  //printf("f %f df %f %f %f\n", f, df[0], df[1], df[2]);

  if (grad)
  {
    grad[0] = df[0];
    grad[1] = df[1];
    grad[2] = df[2];
  }
  
  return f;
}


