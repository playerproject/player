/**************************************************************************
 * Desc: Sensor/action model odometry.
 * Author: Andrew Howard
 * Date: 15 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "odometry.h"


// Build a list of all empty cells in c-space
void odometry_init_cspace(odometry_t *sensor);


// Create an sensor model
odometry_t *odometry_alloc(map_t *map, double robot_radius)
{
  odometry_t *sensor;

  sensor = calloc(1, sizeof(odometry_t));

  sensor->map = map;
  sensor->robot_radius = robot_radius;
  
  sensor->stall = 0;

  sensor->init_gpdf = NULL;
  sensor->init_dpdf = NULL;
  sensor->action_pdf = NULL;

  odometry_init_cspace(sensor);
  
  return sensor;
}


// Free an sensor model
void odometry_free(odometry_t *sensor)
{
  free(sensor->ccells);
  free(sensor);
  return;
}


// Build a list of all empty cells in c-space
void odometry_init_cspace(odometry_t *sensor)
{
  int i, j;
  map_cell_t *cell;
  pf_vector_t *ccell;

  sensor->ccell_count = 0;
  sensor->ccells = malloc(sensor->map->size_x * sensor->map->size_y * sizeof(sensor->ccells[0]));
    
  for (j = 0; j < sensor->map->size_y; j++)
  {
    for (i = 0; i < sensor->map->size_x; i++)
    {
      cell = sensor->map->cells + MAP_INDEX(sensor->map, i, j);

      if (cell->occ_state != -1)
        continue;
      if (cell->occ_dist < sensor->robot_radius)
        continue;

      ccell = sensor->ccells + sensor->ccell_count++;

      ccell->v[0] = MAP_WXGX(sensor->map, i);
      ccell->v[1] = MAP_WYGY(sensor->map, j);
      ccell->v[2] = 0.0;
    }
  }
  
  return;
}


// Set the initial pose
void odometry_init_pose(odometry_t *sensor, pf_vector_t pose, pf_matrix_t pose_cov)
{
  int i;
  pf_pdf_gaussian_t *gpdf;
  double *weights;
  pf_vector_t *ccell;

  if (sensor->init_gpdf)
  {
    pf_pdf_gaussian_free(sensor->init_gpdf);
    sensor->init_gpdf = NULL;
  }

  if (sensor->init_dpdf)
  {
    pf_pdf_discrete_free(sensor->init_dpdf);
    sensor->init_dpdf = NULL;
  }
  
  // Create an array to put weights in
  weights = malloc(sensor->ccell_count * sizeof(weights[0]));

  // Create temporary gaussian pdf 
  sensor->init_gpdf = pf_pdf_gaussian_alloc(pose, pose_cov);

  // Determine the weight for each free cell, based on the gaussian pdf
  for (i = 0; i < sensor->ccell_count; i++)
  {
    ccell = sensor->ccells + i;
    weights[i] = pf_pdf_gaussian_value(sensor->init_gpdf, *ccell);
  }
    
  // Create a discrete pdf
  sensor->init_dpdf = pf_pdf_discrete_alloc(sensor->ccell_count, weights);

  // Free temp stuff
  free(weights);

  return;
}


// The initialization model function
pf_vector_t odometry_init_model(odometry_t *sensor)
{
  int i;
  pf_vector_t pose, npose;
    
  pose = pf_vector_zero();

  // Guess a pose from the discrete distribution
  i = pf_pdf_discrete_sample(sensor->init_dpdf);
  assert(i >= 0 && i < sensor->ccell_count);
  pose = sensor->ccells[i];

  // Perturb with an orientation drawn from the gaussian distribution
  npose = pf_pdf_gaussian_sample(sensor->init_gpdf);
  pose.v[0] += (0.5 - (double) rand() / RAND_MAX) * sensor->map->scale;
  pose.v[1] += (0.5 - (double) rand() / RAND_MAX) * sensor->map->scale;
  pose.v[2] += npose.v[2];  

  //printf("%f %f %f\n", pose.v[0], pose.v[1], pose.v[2]);
      
  return pose;
}


// Set the new odometric pose
void odometry_set_pose(odometry_t *sensor, pf_vector_t old_pose, pf_vector_t new_pose)
{
  pf_vector_t x;
  pf_matrix_t cx;
  double ux, uy, ua;
  
  if (sensor->action_pdf)
  {
    pf_pdf_gaussian_free(sensor->action_pdf);
    sensor->action_pdf = NULL;
  }

  x = pf_vector_coord_sub(new_pose, old_pose);

  // HACK - FIX
  ux = 0.1 * x.v[0];
  uy = 0.1 * x.v[1];
  ua = fabs(0.1 * x.v[2]) + fabs(0.1 * x.v[0]);

  cx = pf_matrix_zero();
  cx.m[0][0] = ux * ux;
  cx.m[1][1] = uy * uy;
  cx.m[2][2] = ua * ua;

  //printf("x = %f %f %f\n", x.v[0], x.v[1], x.v[2]);
  
  // Create a pdf with suitable characterisitics
  sensor->action_pdf = pf_pdf_gaussian_alloc(x, cx); 

  return;
}


// Set the stall bit
void odometry_set_stall(odometry_t *sensor, int stall)
{
  sensor->stall = stall;
  return;
}


// The action model function
pf_vector_t odometry_action_model(odometry_t *sensor, pf_vector_t pose)
{
  pf_vector_t z, npose;
  
  z = pf_pdf_gaussian_sample(sensor->action_pdf);
  npose = pf_vector_coord_add(z, pose);
    
  return npose; 
}


// The sensor model function
double odometry_sensor_model(odometry_t *sensor, pf_vector_t pose)
{  
  double p;
  map_cell_t *cell;  

  cell = map_get_cell(sensor->map, pose.v[0], pose.v[1], pose.v[2]);
  if (!cell)
    return 0;

  if (cell->occ_state != -1)
    return 0.01;
  
  if (sensor->stall == 0)
  {
    if (cell->occ_dist < sensor->robot_radius)
      p = 0.01;
    else
      p = 1.0;
  }
  else
  {
    if (cell->occ_dist < sensor->robot_radius)
      p = 1.0;
    else
      p = 0.01;
  }

  //printf("x = %f %f %f p = %f\n", pose.v[0], pose.v[1], pose.v[2], p);
  
  return p;
}

