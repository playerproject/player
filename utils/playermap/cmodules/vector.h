
/**************************************************************************
 * Desc: Vector functions
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef VECTOR_H
#define VECTOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

 
/**************************************************************************
 * Basic vector and matrix types
 **************************************************************************/

// The basic vector
typedef struct
{
  double v[3];
} vector_t;


// The basic matrix
typedef struct
{
  double m[3][3];
} matrix_t;



/**************************************************************************
 * Vector fuctions
 **************************************************************************/

// Return a zero vector
vector_t vector_zero();

// Check for NAN or INF in any component
int vector_test_finite(vector_t a);

// Print a vector
void vector_fprintf(vector_t s, FILE *file, const char *fmt);

// Simple vector addition
vector_t vector_add(vector_t a, vector_t b);

// Simple vector subtraction
vector_t vector_sub(vector_t a, vector_t b);

// Transform from local to global coords (a + b)
vector_t vector_coord_add(vector_t a, vector_t b);

// Transform from global to local coords (a - b)
vector_t vector_coord_sub(vector_t a, vector_t b);


/**************************************************************************
 * Matrix fuctions
 **************************************************************************/

// Return a zero matrix
matrix_t matrix_zero();

// Check for NAN or INF in any component
int matrix_test_finite(matrix_t a);

// Print a matrix
void matrix_fprintf(matrix_t s, FILE *file, const char *fmt);

// Compute the matrix inverse.  Will also return the determinant,
// which should be checked for underflow (indicated singular matrix).
matrix_t matrix_inverse(matrix_t a, double *det);

// Decompose a covariance matrix [a] into a rotation matrix [r] and a
// diagonal matrix [d] such that a = r * d * r^T.
void matrix_unitary(matrix_t *r, matrix_t *d, matrix_t a);

#ifdef __cplusplus
}
#endif

#endif
