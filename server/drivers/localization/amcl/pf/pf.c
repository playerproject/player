
/**************************************************************************
 * Desc: Simple particle filter for localization.
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "pf.h"
#include "pf_pdf.h"
#include "pf_kdtree.h"


// Compute the required number of samples, given that there are k bins
// with samples in them.
int pf_resample_limit(pf_t *pf, int k);


// Create a new filter
pf_t *pf_alloc(int min_samples, int max_samples)
{
  int i, j;
  pf_t *pf;
  pf_sample_set_t *set;
  pf_sample_t *sample;
  
  pf = calloc(1, sizeof(pf_t));

  pf->min_samples = min_samples;
  pf->max_samples = max_samples;

  // Control parameters for the population size calculation.  [err] is
  // the max error between the true distribution and the estimated
  // distribution.  [z] is the upper standard normal quantile for (1 -
  // p), where p is the probability that the error on the estimated
  // distrubition will be less than [err].
  pf->pop_err = 0.01;
  pf->pop_z = 3;
  
  pf->current_set = 0;
  for (j = 0; j < 2; j++)
  {
    set = pf->sets + j;
      
    set->sample_count = max_samples;
    set->samples = calloc(max_samples, sizeof(pf_sample_t));

    for (i = 0; i < set->sample_count; i++)
    {
      sample = set->samples + i;
      sample->pose.v[0] = 0.0;
      sample->pose.v[1] = 0.0;
      sample->pose.v[2] = 0.0;
      sample->weight = 1.0 / max_samples;
    }

    // HACK: is 3 times max_samples enough?
    set->kdtree = pf_kdtree_alloc(3 * max_samples);
  }

  return pf;
}


// Free an existing filter
void pf_free(pf_t *pf)
{
  int i;
  
  for (i = 0; i < 2; i++)
  {
    free(pf->sets[i].kdtree);
    free(pf->sets[i].samples);
  }
  free(pf);
  
  return;
}


// Initialize the filter using some model
void pf_init(pf_t *pf, pf_init_model_fn_t init_fn, void *init_data)
{
  int i;
  pf_sample_set_t *set;
  pf_sample_t *sample;

  set = pf->sets + pf->current_set;

  set->sample_count = pf->max_samples;

  // Compute the new sample poses
  for (i = 0; i < set->sample_count; i++)
  {
    sample = set->samples + i;
    sample->weight = 1.0 / pf->max_samples;
    sample->pose = (*init_fn) (init_data);
  }
  
  return;
}


/* REMOVE
// Get a particular sample
pf_sample_t *pf_get_sample(pf_t *pf, int i)
{
  pf_sample_set_t *set;

  set = pf->sets + pf->current_set;

  if (i < 0 || i >= set->sample_count)
    return NULL;

  return set->samples + i;
}
*/


// Update the filter with some new action
void pf_update_action(pf_t *pf, pf_action_model_fn_t action_fn, void *action_data)
{
  int i;
  pf_sample_set_t *set;
  pf_sample_t *sample;

  set = pf->sets + pf->current_set;

  // Compute the new sample poses
  for (i = 0; i < set->sample_count; i++)
  {
    sample = set->samples + i;
    sample->pose = (*action_fn) (action_data, sample->pose);
    sample->weight = 1.0 / set->sample_count;
  }
  
  return;
}


// Update the filter with some new sensor observation
void pf_update_sensor(pf_t *pf, pf_sensor_model_fn_t sensor_fn, void *sensor_data)
{
  int i;
  pf_sample_set_t *set;
  pf_sample_t *sample;
  double total;

  set = pf->sets + pf->current_set;
  total = 0;

  // Compute the sample weights
  for (i = 0; i < set->sample_count; i++)
  {
    sample = set->samples + i;
    sample->weight *= (*sensor_fn) (sensor_data, sample->pose);
    total += sample->weight;
  }
  
  //printf("sensor total %e\n", total);
  assert(total > 0);
    
  // Normalize weights
  for (i = 0; i < set->sample_count; i++)
  {
    sample = set->samples + i;
    sample->weight /= total;
  }
  
  return;
}


// Resample the distribution
void pf_update_resample(pf_t *pf)
{
  int i;
  double total;
  double *randlist;
  pf_sample_set_t *set_a, *set_b;
  pf_sample_t *sample_a, *sample_b;
  pf_pdf_discrete_t *pdf;

  set_a = pf->sets + pf->current_set;
  set_b = pf->sets + (pf->current_set + 1) % 2;

  // Create the discrete distribution to sample from
  total = 0;
  randlist = calloc(set_a->sample_count, sizeof(double));
  for (i = 0; i < set_a->sample_count; i++)
  {
    total += set_a->samples[i].weight;
    randlist[i] = set_a->samples[i].weight;
  }

  //printf("resample total %f\n", total);

  // Initialize the random number generator
  pdf = pf_pdf_discrete_alloc(set_a->sample_count, randlist);

  // Create the kd tree for adaptive sampling
  pf_kdtree_clear(set_b->kdtree);

  // Draw samples from set a to create set b.
  total = 0;
  set_b->sample_count = 0;
  while (set_b->sample_count < pf->max_samples)
  {
    i = pf_pdf_discrete_sample(pdf);    
    sample_a = set_a->samples + i;

    //printf("%d %f\n", i, sample_a->weight);
    assert(sample_a->weight > 0);

    // Add sample to list
    sample_b = set_b->samples + set_b->sample_count++;
    sample_b->pose = sample_a->pose;
    sample_b->weight = 1.0;
    total += sample_b->weight;

    // Add sample to histogram
    pf_kdtree_insert(set_b->kdtree, sample_b->pose, sample_b->weight);

    // See if we have enough samples yet
    if (set_b->sample_count >= pf->min_samples)
      if (set_b->sample_count > pf_resample_limit(pf, set_b->kdtree->leaf_count))
        break;
  }

  // TESTING
  //printf("samples %d\n", set_b->sample_count);
  //printf("tree %d\n", tree->node_count);

  pf_pdf_discrete_free(pdf);
  free(randlist);
  
  // Normalize weights
  for (i = 0; i < set_b->sample_count; i++)
  {
    sample_b = set_b->samples + i;
    sample_b->weight /= total;
  }

  // Use the newly created sample set
  pf->current_set = (pf->current_set + 1) % 2;
  
  return;
}


// Compute the required number of samples, given that there are k bins
// with samples in them.  This is taken directly from Fox et al.
int pf_resample_limit(pf_t *pf, int k)
{
  //double z, err;
  double a, b, c, x;
  int n;

  if (k <= 1)
    return pf->min_samples;

  a = 1;
  b = 2 / (9 * ((double) k - 1));
  c = sqrt(2 / (9 * ((double) k - 1))) * pf->pop_z;
  x = a - b + c;

  n = (int) ceil((k - 1) / (2 * pf->pop_err) * x * x * x);

  return n;
}


// Compute the distributions statistics (mean and covariance).
// Assumes sample weights are normalized.
void pf_calc_stats(pf_t *pf, pf_vector_t *mean, pf_matrix_t *cov)
{
  int i, j, k;
  pf_sample_set_t *set;
  pf_sample_t *sample;
  //double r;
  double n;
  double m[4];
  double c[2][2];

  set = pf->sets + pf->current_set;

  n = 0.0;
  
  // Initialise mean to zero
  for (j = 0; j < 4; j++)
    m[j] = 0.0;

  // Initialize covariance
  for (j = 0; j < 2; j++)
    for (k = 0; k < 2; k++)
      c[j][k] = 0.0;
  
  for (i = 0; i < set->sample_count; i++)
  {
    sample = set->samples + i;

    // Compute mean
    n += sample->weight;
    m[0] += sample->weight * sample->pose.v[0];
    m[1] += sample->weight * sample->pose.v[1];
    m[2] += sample->weight * cos(sample->pose.v[2]);
    m[3] += sample->weight * sin(sample->pose.v[2]);

    // Compute covariance in linear components
    for (j = 0; j < 2; j++)
      for (k = 0; k < 2; k++)
        c[j][k] += sample->weight * sample->pose.v[j] * sample->pose.v[k];
  }

  mean->v[0] = m[0] / n;
  mean->v[1] = m[1] / n;
  mean->v[2] = atan2(m[3], m[2]);

  *cov = pf_matrix_zero();

  // Covariance in linear compontents
  for (j = 0; j < 2; j++)
    for (k = 0; k < 2; k++)
      cov->m[j][k] = c[j][k] / n - mean->v[j] * mean->v[k];

  // Covariance in angular components; I think this is the correct
  // formular for circular statistics.
  cov->m[2][2] = -2 * log(sqrt(m[2] * m[2] + m[3] * m[3]));
  
  return;
}


