
/**************************************************************************
 * Desc: Vector functions
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <math.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_linalg.h>
#include "pf_vector.h"


// Simple vector addition
pf_vector_t pf_vector_add(pf_vector_t a, pf_vector_t b)
{
  pf_vector_t c;

  c.v[0] = a.v[0] + b.v[0];
  c.v[1] = a.v[1] + b.v[1];
  c.v[2] = a.v[2] + b.v[2];
  
  return c;
}


// Simple vector subtraction
pf_vector_t pf_vector_sub(pf_vector_t a, pf_vector_t b)
{
  pf_vector_t c;

  c.v[0] = a.v[0] - b.v[0];
  c.v[1] = a.v[1] - b.v[1];
  c.v[2] = a.v[2] - b.v[2];
  
  return c;
}


// Transform from local to global coords (a + b)
pf_vector_t pf_vector_coord_add(pf_vector_t a, pf_vector_t b)
{
  pf_vector_t c;

  c.v[0] = b.v[0] + a.v[0] * cos(b.v[2]) - a.v[1] * sin(b.v[2]);
  c.v[1] = b.v[1] + a.v[0] * sin(b.v[2]) + a.v[1] * cos(b.v[2]);
  c.v[2] = b.v[2] + a.v[2];
  
  return c;
}


// Transform from global to local coords (a - b)
pf_vector_t pf_vector_coord_sub(pf_vector_t a, pf_vector_t b)
{
  pf_vector_t c;

  c.v[0] = +(a.v[0] - b.v[0]) * cos(b.v[2]) + (a.v[1] - b.v[1]) * sin(b.v[2]);
  c.v[1] = -(a.v[0] - b.v[0]) * sin(b.v[2]) + (a.v[1] - b.v[1]) * cos(b.v[2]);
  c.v[2] = a.v[2] - b.v[2];
  
  return c;
}


// Compute the matrix inverse
pf_matrix_t pf_matrix_inverse(pf_matrix_t a, double *det)
{
  double lndet;
  int signum;
  gsl_permutation *p;
  gsl_matrix_view A, Ai;

  pf_matrix_t ai;

  A = gsl_matrix_view_array((double*) a.m, 3, 3);
  Ai = gsl_matrix_view_array((double*) ai.m, 3, 3);
  
  // Do LU decomposition
  p = gsl_permutation_alloc(3);
  gsl_linalg_LU_decomp(&A.matrix, p, &signum);

  // Check for underflow
  lndet = gsl_linalg_LU_lndet(&A.matrix);
  if (lndet < -1000)
  {
    //printf("underflow in matrix inverse lndet = %f", lndet);
    gsl_matrix_set_zero(&Ai.matrix);
  }
  else
  {
    // Compute inverse
    gsl_linalg_LU_invert(&A.matrix, p, &Ai.matrix);
  }

  gsl_permutation_free(p);

  if (det)
    *det = exp(lndet);

  return ai;
}


// Decompose a covariance matrix [a] into a rotation matrix [r] and a diagonal
// matrix [d] such that a = r d r^T.
// TODO: there may be a better way of doing this.
void pf_matrix_svd(pf_matrix_t *r, pf_matrix_t *d, pf_matrix_t a)
{
  int i, j;
  gsl_matrix *u, *v;
  gsl_vector *s;
  gsl_vector *work;
  
  u = gsl_matrix_alloc(3, 3);
  v = gsl_matrix_alloc(3, 3);
  s = gsl_vector_alloc(3);
  work = gsl_vector_alloc(3);

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      gsl_matrix_set(u, i, j, a.m[i][j]);
  
  gsl_linalg_SV_decomp(u, v, s, work);

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      r->m[i][j] = gsl_matrix_get(u, i, j);

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      d->m[i][j] = 0;
  
  for (i = 0; i < 3; i++)
    d->m[i][i] = gsl_vector_get(s, i);
  
  gsl_vector_free(work);
  gsl_vector_free(s);
  gsl_matrix_free(v);
  gsl_matrix_free(u);  
  
  return;
}

