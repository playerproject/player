
/**************************************************************************
 * Desc: Simple particle filter for localization.
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef PF_H
#define PF_H

#include "pf_vector.h"

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct _pf_t;
struct _rtk_fig_t;

// Function prototype for the initialization model; generates a sample pose from
// an appropriate distribution.
typedef pf_vector_t (*pf_init_model_fn_t) (void *init_data);

// Function prototype for the action model; generates a sample pose from
// an appropriate distribution
typedef pf_vector_t (*pf_action_model_fn_t) (void *action_data, pf_vector_t pose);

// Function prototype for the sensor model; determines the probability
// for the given sample pose.
typedef double (*pf_sensor_model_fn_t) (void *sensor_data, pf_vector_t pose);


// Information for a single sample
typedef struct
{
  // Pose represented by this sample
  pf_vector_t pose;

  // Weight for this pose
  double weight;
  
} pf_sample_t;


// Information for a set of samples
typedef struct
{
  int sample_count;
  pf_sample_t *samples;
} pf_sample_set_t;


// Information for an entire filter
typedef struct _pf_t
{
  // This min and max number of samples
  int min_samples, max_samples;
  
  // The sample sets.  We keep two sets an use [current_set]
  // to identify the active set.
  int current_set;
  pf_sample_set_t sets[2];
  
} pf_t;


// Create a new filter
pf_t *pf_alloc(int min_samples, int max_samples);

// Free an existing filter
void pf_free(pf_t *pf);

// Initialize the filter using some model
void pf_init(pf_t *pf, pf_init_model_fn_t init_fn, void *init_data);

// Update the filter with some new action
void pf_update_action(pf_t *pf, pf_action_model_fn_t action_fn, void *action_data);

// Update the filter with some new sensor observation
void pf_update_sensor(pf_t *pf, pf_sensor_model_fn_t sensor_fn, void *sensor_data);

// Resample the distribution
void pf_update_resample(pf_t *pf);

// Compute the distributions statistics (mean and covariance).
void pf_calc_stats(pf_t *pf, pf_vector_t *mean, pf_matrix_t *cov);

// Get a particular sample
//REMOVE pf_sample_t *pf_get_sample(pf_t *pf, int i);

// Display the sample set
void pf_draw_samples(pf_t *pf, struct _rtk_fig_t *fig, int max_samples);

#ifdef __cplusplus
}
#endif


#endif
