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


// Create an sensor model
laser_t *laser_alloc(map_t *map, pf_vector_t laser_pose)
{
  laser_t *sensor;

  sensor = calloc(1, sizeof(laser_t));

  sensor->map = map;
  sensor->laser_pose = laser_pose;
  
  sensor->range_count = 0;
  sensor->ranges = calloc(LASER_MAX_RANGES, sizeof(laser_range_t));
  
  return sensor;
}


// Free an sensor model
void laser_free(laser_t *sensor)
{
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


// Determine the probability for the given pose
double laser_sensor_model(laser_t *sensor, pf_vector_t pose)
{
  int i;
  double p;
  double model_range;
  double z, zz, u, cov;
  laser_range_t *obs;

  cov = 0.05 * 0.05;

  // Take account of the laser pose relative to the robot
  pose = pf_vector_coord_add(sensor->laser_pose, pose);
  
  u = 0;
  for (i = 0; i < sensor->range_count; i++)
  {
    obs = sensor->ranges + i;
    
    model_range = map_calc_range(sensor->map,
                                 pose.v[0], pose.v[1], pose.v[2] + obs->bearing, 8.0);

    //printf("%d %f %f %f\n", i, obs->bearing * 180 / M_PI, obs->range, model_range);

    z = (obs->range - model_range);
    zz = z * z;
    u += zz;
  }

  //printf("\n\n");

  // TODO: proper (and faster) model
  //p = 0.20 + 0.80 * (1 / (2 * M_PI * cov) * exp(-u / (2 * cov)));

  p = (1 / (2 * M_PI * cov) * exp(-u / (2 * cov)));

  //printf("%e\n", p);
  assert(p >= 0);

  return p;
}

