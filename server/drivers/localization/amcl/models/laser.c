/**************************************************************************
 * Desc: Sensor model the laser sensor.
 * Author: Andrew Howard
 * Date: 15 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "laser.h"

#define LASER_MAX_RANGES 401

// Pre-compute the range sensor probabilities
void laser_precompute(laser_t *sensor);


// Create an sensor model
laser_t *laser_alloc(map_t *map, pf_vector_t laser_pose)
{
  laser_t *sensor;

  sensor = calloc(1, sizeof(laser_t));

  sensor->map = map;
  sensor->laser_pose = laser_pose;

  sensor->range_cov = 0.05 * 0.05;
  sensor->range_bad = 0.20;

  laser_precompute(sensor);
    
  sensor->range_count = 0;
  sensor->ranges = calloc(LASER_MAX_RANGES, sizeof(laser_range_t));
  
  return sensor;
}


// Free an sensor model
void laser_free(laser_t *sensor)
{
  free(sensor->lut_probs);
  free(sensor->ranges);
  free(sensor);
  return;
}


// Clear all existing range readings
void laser_clear_ranges(laser_t *sensor)
{
  sensor->range_count = 0;
  return;
}


// Set the laser range readings that will be used.
void laser_add_range(laser_t *sensor, double range, double bearing)
{
  laser_range_t *beam;
  
  assert(sensor->range_count < LASER_MAX_RANGES);
  beam = sensor->ranges + sensor->range_count++;
  beam->range = range;
  beam->bearing = bearing;

  return;
}


// Pre-compute the range sensor probabilities.
// We use a two-dimensional array over (model_range, obs_range).
// currently, only the difference (obs_range - model_range) is significant,
// so this is somewhat inefficient.
void laser_precompute(laser_t *sensor)
{
  double max;
  double c, z, p;
  double mrange, orange;
  int i, j;
  
  // Laser max range and resolution
  max = 8.00;
  sensor->lut_res = 0.01;
  
  sensor->lut_size = (int) ceil(max / sensor->lut_res);
  sensor->lut_probs = malloc(sensor->lut_size * sensor->lut_size * sizeof(sensor->lut_probs[0]));

  for (i = 0; i < sensor->lut_size; i++)
  {
    mrange = i * sensor->lut_res;
    
    for (j = 0; j < sensor->lut_size; j++)
    {
      orange = j * sensor->lut_res;

      // TODO: proper sensor model (using Kolmagorov?)
      // Simple gaussian model
      c = sensor->range_cov;
      z = orange - mrange;
      p = sensor->range_bad + (1 - sensor->range_bad) *
        (1 / (sqrt(2 * M_PI * c)) * exp(-(z * z) / (2 * c)));

      sensor->lut_probs[i + j * sensor->lut_size] = p;
    }
  }

  return;
}


// Determine the probability for the given range reading
inline double laser_sensor_prob(laser_t *sensor, double obs_range, double map_range)
{
  int i, j;

  i = (int) (map_range / sensor->lut_res + 0.5);
  j = (int) (obs_range / sensor->lut_res + 0.5);

  if (i < 0 || i >= sensor->lut_size)
    return 1.0;
  if (j < 0 || j >= sensor->lut_size)
    return 1.0;

  return sensor->lut_probs[i + j * sensor->lut_size];
}


// Determine the probability for the given pose
double laser_sensor_model(laser_t *sensor, pf_vector_t pose)
{
  int i;
  double p;
  double map_range;
  laser_range_t *obs;

  // Take account of the laser pose relative to the robot
  pose = pf_vector_coord_add(sensor->laser_pose, pose);

  p = 1.0;
  
  for (i = 0; i < sensor->range_count; i++)
  {
    obs = sensor->ranges + i;
    map_range = map_calc_range(sensor->map,
                               pose.v[0], pose.v[1], pose.v[2] + obs->bearing, 8.0);

    p *= laser_sensor_prob(sensor, obs->range, map_range);
  }

  //printf("%e\n", p);
  assert(p >= 0);

  return p;
}

