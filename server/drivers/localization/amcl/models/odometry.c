/**************************************************************************
 * Desc: Sensor/action model odometry.
 * Author: Andrew Howard
 * Date: 15 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <math.h>
#include <stdlib.h>

#include "odometry.h"

void odometry_init(odometry_t *sensor);

// Create an sensor model
odometry_t *odometry_alloc(map_t *map)
{
  odometry_t *sensor;

  sensor = calloc(1, sizeof(odometry_t));

  sensor->map = map;
  sensor->stall = 0;

  sensor->init_pdf = NULL;
  sensor->action_pdf = NULL;
  
  return sensor;
}


// Free an sensor model
void odometry_free(odometry_t *sensor)
{
  free(sensor);
  return;
}


// Set the initial pose
void odometry_init_pose(odometry_t *sensor, pf_vector_t pose, pf_matrix_t pose_cov)
{
  if (sensor->init_pdf)
  {
    pf_pdf_gaussian_free(sensor->init_pdf);
    sensor->init_pdf = NULL;
  }
  
  // Create a pdf with suitable characterisitics
  sensor->init_pdf = pf_pdf_gaussian_alloc(pose, pose_cov); 

  return;
}


// The initialization model function
pf_vector_t odometry_init_model(odometry_t *sensor)
{
  pf_vector_t pose;
  map_cell_t *cell;

  pose.v[0] = pose.v[1] = pose.v[2] = 0;

  while (1)
  {
    pose = pf_pdf_gaussian_sample(sensor->init_pdf);
    //printf("%f %f %f\n", pose.v[0], pose.v[1], pose.v[2]);
    cell = map_get_cell(sensor->map, pose.v[0], pose.v[1], pose.v[2]);
    if (cell && cell->occ_dist > 0.25) //TESTING
        break;
  }
      
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
  ua = 0.1 * x.v[2] + 0.1 * x.v[0];
  cx.m[0][0] = ux * ux;
  cx.m[0][1] = 0.0;
  cx.m[0][2] = 0.0;
  cx.m[1][0] = 0.0;
  cx.m[1][1] = uy * uy;
  cx.m[1][2] = 0.0;
  cx.m[2][0] = 0.0;
  cx.m[2][1] = 0.0;
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

  if (sensor->stall == 0)
  {
    // TESTING
    if (cell->occ_dist < 0.25)
      p = 0.01;
    else
      p = 1.0;
  }
  else
  {
    // TESTING
    if (cell->occ_dist < 0.25)
      p = 1.0;
    else
      p = 0.01;
  }

  //printf("x = %f %f %f p = %f\n", pose.v[0], pose.v[1], pose.v[2], p);
  
  return p;
}

