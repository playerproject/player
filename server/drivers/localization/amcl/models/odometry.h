
/**************************************************************************
 * Desc: Sensor/action models for odometry.
 * Author: Andrew Howard
 * Date: 15 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef ODOMETRY_H
#define ODOMETRY_H

#include "../pf/pf.h"
#include "../pf/pf_pdf.h"
#include "../map/map.h"

#ifdef __cplusplus
extern "C" {
#endif

  
// Model information
typedef struct
{
  // Pointer to the map map
  map_t *map;

  // Effective robot radius
  double robot_radius;

  // List of free cells in cspace
  int ccell_count;
  pf_vector_t *ccells;
  
  // PDFs used to generate initial samples
  pf_pdf_gaussian_t *init_gpdf;
  pf_pdf_discrete_t *init_dpdf;
  
  // PDF used to generate action samples
  pf_pdf_gaussian_t *action_pdf;

} odometry_t;


// Create an sensor model
odometry_t *odometry_alloc(map_t *map, double robot_radius);

// Free an sensor model
void odometry_free(odometry_t *sensor);

// Build a list of all empty cells in c-space
int odometry_init_cspace(odometry_t *self);

  
// Prepare to initialize the distribution; pose is the robot's initial
// pose estimate.
void odometry_init_init(odometry_t *self, pf_vector_t pose, pf_matrix_t pose_cov);

// Finish initializing the distribution
void odometry_init_term(odometry_t *self);

// The initialization model function
pf_vector_t odometry_init_model(odometry_t *self);


// Prepare to update the distribution using the action model.
void odometry_action_init(odometry_t *self, pf_vector_t old_pose, pf_vector_t new_pose);

// Finish updating the distrubiotn using the action model
void odometry_action_term(odometry_t *self);

// The action model function
pf_vector_t odometry_action_model(odometry_t *self, pf_vector_t pose);

  
// Prepare to update the distribution using the sensor model.
void odometry_sensor_init(odometry_t *self);

// Finish updating the distribution using the sensor model.
void odometry_sensor_term(odometry_t *self);

// The sensor model function
double odometry_sensor_model(odometry_t *self, pf_vector_t pose);

#ifdef __cplusplus
}
#endif

#endif

