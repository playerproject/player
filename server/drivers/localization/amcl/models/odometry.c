/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
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


// Create an sensor model
odometry_t *odometry_alloc(map_t *map, double robot_radius)
{
  odometry_t *self;

  self = calloc(1, sizeof(odometry_t));

  self->map = map;
  self->robot_radius = robot_radius;
  
  return self;
}


// Free an sensor model
void odometry_free(odometry_t *self)
{
  free(self->ccells);
  free(self);
  return;
}


// Build a list of all empty cells in c-space
int odometry_init_cspace(odometry_t *self)
{
  int i, j;
  map_cell_t *cell;
  pf_vector_t *ccell;

  if (self->map == NULL)
    return 0;

  self->ccell_count = 0;
  self->ccells = malloc(self->map->size_x * self->map->size_y * sizeof(self->ccells[0]));
    
  for (j = 0; j < self->map->size_y; j++)
  {
    for (i = 0; i < self->map->size_x; i++)
    {
      cell = self->map->cells + MAP_INDEX(self->map, i, j);

      if (cell->occ_state != -1)
        continue;
      if (cell->occ_dist < self->robot_radius)
        continue;

      ccell = self->ccells + self->ccell_count++;

      ccell->v[0] = MAP_WXGX(self->map, i);
      ccell->v[1] = MAP_WYGY(self->map, j);
      ccell->v[2] = 0.0;
    }
  }

  if (self->ccell_count == 0)
    return -1;
  return 0;
}


// Prepare to initialize the distribution; pose is the robot's initial
// pose estimate.
void odometry_init_init(odometry_t *self, pf_vector_t pose, pf_matrix_t pose_cov)
{
  int i;
  double *weights;
  pf_vector_t *ccell;
  
  // Create an array to put weights in
  weights = malloc(self->ccell_count * sizeof(weights[0]));

  // Create temporary gaussian pdf 
  self->init_gpdf = pf_pdf_gaussian_alloc(pose, pose_cov);

  // Determine the weight for each free cell, based on the gaussian pdf
  for (i = 0; i < self->ccell_count; i++)
  {
    ccell = self->ccells + i;
    weights[i] = pf_pdf_gaussian_value(self->init_gpdf, *ccell);
  }
    
  // Create a discrete pdf
  self->init_dpdf = pf_pdf_discrete_alloc(self->ccell_count, weights);

  // Free temp stuff
  free(weights);

  return;
}


// Finish initializing the distribution
void odometry_init_term(odometry_t *self)
{
  pf_pdf_gaussian_free(self->init_gpdf);
  pf_pdf_discrete_free(self->init_dpdf);

  self->init_gpdf = NULL;
  self->init_dpdf = NULL;
  return;
}


// The initialization model function
pf_vector_t odometry_init_model(odometry_t *self)
{
  int i;
  pf_vector_t pose, npose;
    
  pose = pf_vector_zero();

  // Guess a pose from the discrete distribution
  i = pf_pdf_discrete_sample(self->init_dpdf);
  assert(i >= 0 && i < self->ccell_count);
  pose = self->ccells[i];

  // Perturb with an orientation drawn from the gaussian distribution
  npose = pf_pdf_gaussian_sample(self->init_gpdf);
  pose.v[0] += (0.5 - (double) rand() / RAND_MAX) * self->map->scale;
  pose.v[1] += (0.5 - (double) rand() / RAND_MAX) * self->map->scale;
  pose.v[2] += npose.v[2];  

  //printf("%f %f %f\n", pose.v[0], pose.v[1], pose.v[2]);
      
  return pose;
}


// Prepare to update the distribution using the action model.
void odometry_action_init(odometry_t *self, pf_vector_t old_pose, pf_vector_t new_pose)
{
  pf_vector_t x;
  pf_matrix_t cx;
  double ux, uy, ua;
  
  x = pf_vector_coord_sub(new_pose, old_pose);

  // HACK - FIX
  ux = 0.2 * x.v[0];
  uy = 0.2 * x.v[1];
  ua = fabs(0.2 * x.v[2]) + fabs(0.2 * x.v[0]);

  cx = pf_matrix_zero();
  cx.m[0][0] = ux * ux;
  cx.m[1][1] = uy * uy;
  cx.m[2][2] = ua * ua;

  //printf("x = %f %f %f\n", x.v[0], x.v[1], x.v[2]);
  
  // Create a pdf with suitable characterisitics
  self->action_pdf = pf_pdf_gaussian_alloc(x, cx); 

  return;
}


// Finish updating the distrubiotn using the action model
void odometry_action_term(odometry_t *self)
{
  pf_pdf_gaussian_free(self->action_pdf);
  self->action_pdf = NULL;
  return;
}


// The action model function
pf_vector_t odometry_action_model(odometry_t *self, pf_vector_t pose)
{
  pf_vector_t z, npose;
  
  z = pf_pdf_gaussian_sample(self->action_pdf);
  npose = pf_vector_coord_add(z, pose);
    
  return npose; 
}


// Prepare to update the distribution using the sensor model.
void odometry_sensor_init(odometry_t *self)
{
  return;
}


// Finish updating the distribution using the sensor model.
void odometry_sensor_term(odometry_t *self)
{
  return;
}


// The sensor model function
double odometry_sensor_model(odometry_t *self, pf_vector_t pose)
{  
  double p;
  map_cell_t *cell;  

  if (self->map == NULL)
    return 1.0;
    
  cell = map_get_cell(self->map, pose.v[0], pose.v[1], pose.v[2]);
  if (!cell)
    return 0;

  if (cell->occ_state != -1)
    return 0.01;
  
  if (cell->occ_dist < self->robot_radius)
    p = 0.01;
  else
    p = 1.0;

  //printf("x = %f %f %f p = %f\n", pose.v[0], pose.v[1], pose.v[2], p);
  
  return p;
}

