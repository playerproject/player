/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Andrew Howard
 *     Brian Gerkey    
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */


/**************************************************************************
 * Desc: Useful pdf functions
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef PF_PDF_H
#define PF_PDF_H

#include "pf_vector.h"

//#include <gsl/gsl_rng.h>
//#include <gsl/gsl_randist.h>

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 * Gaussian
 *************************************************************************/

// Gaussian PDF info
typedef struct
{
  // Mean, covariance and inverse covariance
  pf_vector_t x;
  pf_matrix_t cx;
  //pf_matrix_t cxi;
  double cxdet;

  // Decomposed covariance matrix (rotation * diagonal)
  pf_matrix_t cr;
  pf_vector_t cd;

  // A random number generator
  //gsl_rng *rng;

} pf_pdf_gaussian_t;


// Create a gaussian pdf
pf_pdf_gaussian_t *pf_pdf_gaussian_alloc(pf_vector_t x, pf_matrix_t cx);

// Destroy the pdf
void pf_pdf_gaussian_free(pf_pdf_gaussian_t *pdf);

// Compute the value of the pdf at some point [z].
//double pf_pdf_gaussian_value(pf_pdf_gaussian_t *pdf, pf_vector_t z);

// Draw randomly from a zero-mean Gaussian distribution, with standard
// deviation sigma.
// We use the polar form of the Box-Muller transformation, explained here:
//   http://www.taygeta.com/random/gaussian.html
double pf_ran_gaussian(double sigma);

// Generate a sample from the the pdf.
pf_vector_t pf_pdf_gaussian_sample(pf_pdf_gaussian_t *pdf);


#if 0

/**************************************************************************
 * Discrete
 *************************************************************************/

// Discrete PDF info
typedef struct
{
  // The list of discrete probs
  int prob_count;
  double *probs;

  // A random number generator
  gsl_rng *rng;

  // The discrete prob generator
  gsl_ran_discrete_t *ran;

} pf_pdf_discrete_t;


// Create a discrete pdf
pf_pdf_discrete_t *pf_pdf_discrete_alloc(int count, double *probs);

// Destroy the pdf
void pf_pdf_discrete_free(pf_pdf_discrete_t *pdf);

// Compute the value of the probability of some element [i]
double pf_pdf_discrete_value(pf_pdf_discrete_t *pdf, int i);

// Generate a sample from the the pdf.
int pf_pdf_discrete_sample(pf_pdf_discrete_t *pdf);
#endif

#ifdef __cplusplus
}
#endif

#endif
