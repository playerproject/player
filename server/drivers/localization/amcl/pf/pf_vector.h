
/**************************************************************************
 * Desc: Vector functions
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef PF_VECTOR_H
#define PF_VECTOR_H


// The basic vector
typedef struct
{
  double v[3];
} pf_vector_t;


// The basic matrix
typedef struct
{
  double m[3][3];
} pf_matrix_t;


// Simple vector addition
pf_vector_t pf_vector_add(pf_vector_t a, pf_vector_t b);

// Simple vector subtraction
pf_vector_t pf_vector_sub(pf_vector_t a, pf_vector_t b);

// Transform from local to global coords (a + b)
pf_vector_t pf_vector_coord_add(pf_vector_t a, pf_vector_t b);

// Transform from global to local coords (a - b)
pf_vector_t pf_vector_coord_sub(pf_vector_t a, pf_vector_t b);

// Compute the matrix inverse.  Will also return the determinant,
// which should be checked for underflow (indicated singular matrix).
pf_matrix_t pf_matrix_inverse(pf_matrix_t a, double *det);

// Decompose a covariance matrix [a] into a rotation matrix [r] and a
// diagonal matrix [d] such that a = r * d * r^T.
void pf_matrix_svd(pf_matrix_t *r, pf_matrix_t *d, pf_matrix_t a);


#endif
