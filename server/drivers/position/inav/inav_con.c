
/**************************************************************************
 * Desc: Local controller
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "inav_con.h"

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

// Generate the trajectory for a particular set of controller settings
static double icon_make_path(icon_t *inav_con, icon_path_t *path, icon_param_t *param);

// Update the robot state
static void icon_robot_model(icon_t *icon, inav_vector_t *pose,
                             inav_vector_t *vel, const inav_vector_t *control);

// Generate the control signal for a particular robot state
static void icon_control_model(icon_t *icon, const inav_vector_t *pose, const inav_vector_t *vel, 
                               inav_vector_t *control, const icon_param_t *params);

// Determine the cost associated with a state
static double icon_cost_model(icon_t *icon, const inav_vector_t *pose, const inav_vector_t *vel);

// Determine the virtual force on the given point
static void icon_force(icon_t *icon, const double p[2], double f[2], const icon_param_t *param);



// Create a new controller
icon_t *icon_alloc(imap_t *map, double min_dist)
{
  icon_t *icon;

  icon = (icon_t*) calloc(1, sizeof(icon_t));

  icon->map = map;
  icon->dt = 0.1;
  icon->pivot = 0.20;
  icon->min_dist = min_dist;
  
  icon->robot_min_vel.v[0] = -0.05;
  icon->robot_min_vel.v[2] = -30 * M_PI / 180;
  icon->robot_max_vel.v[0] = +0.20;
  icon->robot_max_vel.v[2] = +30 * M_PI / 180;

  icon->best_path = NULL;
  icon->path_count = 0;

  return icon;
}


// Destroy the controller
void icon_free(icon_t *icon)
{
  free(icon);
  return;
}


// Set the goal pose
void icon_set_goal(icon_t *icon, double pose[3])
{
  icon->goal_pose.v[0] = pose[0];
  icon->goal_pose.v[1] = pose[1];
  icon->goal_pose.v[2] = pose[2];

  return;
}


// Set the current robot state: pose (relative to the map) and
// velocity.
void icon_set_robot(icon_t *icon, double pose[3], double vel[3])
{
  icon->robot_pose.v[0] = pose[0];
  icon->robot_pose.v[1] = pose[1];
  icon->robot_pose.v[2] = pose[2];
  icon->robot_vel.v[0] = vel[0];
  icon->robot_vel.v[1] = vel[1];
  icon->robot_vel.v[2] = vel[2];

  return;
}


// Compute the control vector
void icon_get_control(icon_t *icon, double control[3])
{
  int i;
  icon_path_t *path;
  icon_param_t *param;

  int param_count;
  icon_param_t params[30];

  // TESTING
  double sets[][4] =
    {
      // This set is for staying still
      {1, 1, 0, 0},

      {1, 1, 0.1, 0.1},
      {1, 1, 1.0, 0.1},
      {1, 1, 0.1, 1.0},
      {1, 1, 1.0, 1.0},

      {1, 0.1, 0.1, 0.1},
      {1, 0.1, 1.0, 0.1},
      {1, 0.1, 0.1, 1.0},
      {1, 0.1, 1.0, 1.0},

      {1, 0.01, 0.1, 0.1},
      {1, 0.01, 1.0, 0.1},
      {1, 0.01, 0.1, 1.0},
      {1, 0.01, 1.0, 1.0},
    };

  // Initialize the parameters
  param_count = 0;
  for (i = 0; i < sizeof(sets) / sizeof(sets[0]); i++)
  {
    params[param_count].id = i;
    params[param_count].kg = sets[i][0];
    params[param_count].ko = sets[i][1];
    params[param_count].pd[0][0] = sets[i][2];
    params[param_count].pd[2][0] = sets[i][3];
    param_count++;
  }

  // Make some random parameter sets
  for (i = 0; i < 4; i++)
  {
    params[param_count].id = i;
    params[param_count].kg = 10.0 * ((double) rand() / RAND_MAX);
    params[param_count].ko = 5.0 * ((double) rand() / RAND_MAX);
    params[param_count].pd[0][0] = 5.0 * ((double) rand() / RAND_MAX);
    params[param_count].pd[2][0] = 5.0 * ((double) rand() / RAND_MAX);
    param_count++;
  }

  icon->best_path = NULL;
  icon->path_count = 0;
    
  for (i = 0; i < param_count; i++)
  {
    param = params + i;
    path = icon->paths + icon->path_count++;
  
    icon_make_path(icon, path, param);

    if (icon->best_path == NULL || path->cost < icon->best_path->cost)
      icon->best_path = path;
  }

  if (icon->best_path)
  {
    icon_control_model(icon, &icon->robot_pose, &icon->robot_vel, 
                       &icon->control, &icon->best_path->param);
  }
  else
  {
    icon->control.v[0] = 0;
    icon->control.v[1] = 0;
    icon->control.v[2] = 0;
  }

  control[0] = icon->control.v[0];
  control[1] = icon->control.v[1];
  control[2] = icon->control.v[2];

  return;
}


// Generate the trajectory for a particular set of controller settings
double icon_make_path(icon_t *icon, icon_path_t *path, icon_param_t *param)
{
  int i, steps;
  double cost;
  inav_vector_t pose, vel, control;

  path->param = *param;
    
  pose = icon->robot_pose;
  vel = icon->robot_vel;

  // HACK
  steps = (int) (10.0 / icon->dt);

  cost = 0;
  path->pose_count = 0;
  
  for (i = 0; i < steps; i++)
  {
    // Update the controller
    icon_control_model(icon, &pose, &vel, &control, param);
    
    // Update the robot state
    icon_robot_model(icon, &pose, &vel, &control);

    // Evaluate the state
    cost += icon_cost_model(icon, &pose, &vel);

    //printf("%f %f %f\n", pose.v[0], pose.v[1], pose.v[2]);
    
    // Add pose to path
    assert(path->pose_count < sizeof(path->poses) / sizeof(path->poses[0]));
    path->poses[path->pose_count++] = pose;
  }

  path->cost = cost;
  
  return cost;
}


// Update the robot state
void icon_robot_model(icon_t *icon, inav_vector_t *pose,
                      inav_vector_t *vel, const inav_vector_t *control)
{
  int i;
  double kp;

  // Equations for differential drive
  pose->v[0] += icon->dt * vel->v[0] * cos(pose->v[2]);
  pose->v[1] += icon->dt * vel->v[0] * sin(pose->v[2]);
  pose->v[2] += icon->dt * vel->v[2];

  // HACK
  kp = 0.05;
    
  // Model robot as a proportional controller
  vel->v[0] += kp * (control->v[0] - vel->v[0]);
  vel->v[1] = 0;
  vel->v[2] += kp * (control->v[2] - vel->v[2]);

  // Clip the velocities
  for (i = 0; i < 3; i++)
  {
    vel->v[i] = MIN(vel->v[i], icon->robot_max_vel.v[i]);
    vel->v[i] = MAX(vel->v[i], icon->robot_min_vel.v[i]);
  }
  
  return;
}


// Generate the control signal for a particular robot state
void icon_control_model(icon_t *icon, const inav_vector_t *pose, const inav_vector_t *vel, 
                        inav_vector_t *control, const icon_param_t *param)
{
  int i;
  double p[2];
  double f[2];
  double g[2], h[2];

  // Compute pivot point
  p[0] = pose->v[0] + icon->pivot * cos(pose->v[2]);
  p[1] = pose->v[1] + icon->pivot * sin(pose->v[2]);
  
  // Compute the virtual force on the pivot point
  icon_force(icon, p, f, param);

  // Rotate force into robot cs
  g[0] = +f[0] * cos(pose->v[2]) + f[1] * sin(pose->v[2]);
  g[1] = -f[0] * sin(pose->v[2]) + f[1] * cos(pose->v[2]);

  // Compute effective force and torque
  h[0] = g[0];
  h[1] = g[1] * icon->pivot;
    
  // Compute the control velocities
  control->v[0] = param->pd[0][0] * h[0];
  control->v[1] = 0.0;
  control->v[2] = param->pd[2][0] * h[1];

  // Clip the velocities
  for (i = 0; i < 3; i++)
  {
    control->v[i] = MAX(control->v[i], icon->robot_min_vel.v[i]);
    control->v[i] = MIN(control->v[i], icon->robot_max_vel.v[i]);
  }

  // Special case: robot is going backwards
  if (control->v[0] < 0)
  {
    if (control->v[2] >= 0)
      control->v[2] = icon->robot_max_vel.v[2];
    else
      control->v[2] = icon->robot_min_vel.v[2];
  }

  return;
}


// Determine the virtual force on the given point
void icon_force(icon_t *icon, const double p[2], double f[2], const icon_param_t *param)
{
  double d[2];
  double dist;
  
  f[0] = 0;
  f[1] = 0;
  
  // Compute the goal force
  d[0] = p[0] - icon->goal_pose.v[0];
  d[1] = p[1] - icon->goal_pose.v[1];
  dist = sqrt(d[0] * d[0] + d[0] * d[0]);

  f[0] += -param->kg * d[0];
  f[1] += -param->kg * d[1];
  
  // Compute the obstacle force
  dist = imap_occ_vector(icon->map, p[0], p[1], d + 0, d + 1);
  
  f[0] += -param->ko * d[0] / (dist * dist * dist);
  f[1] += -param->ko * d[1] / (dist * dist * dist);

  return;
}


// Determine the cost associated with a state
double icon_cost_model(icon_t *icon, const inav_vector_t *pose, const inav_vector_t *vel)
{
  double cost;
  double dx, dy, d;

  cost = 0.0;

  d = imap_occ_dist(icon->map, pose->v[0], pose->v[1]);
  if (d < icon->min_dist)
  {
    // Collision cost
    cost = 1e6 / (icon->min_dist - d + 1e-6);
    cost += 1e6;
    cost += vel->v[0] * vel->v[0];
  }
  else
  {
    // Goal cost
    dx = pose->v[0] - icon->goal_pose.v[0];
    dy = pose->v[1] - icon->goal_pose.v[1];
    d = sqrt(dx * dx + dy * dy);
    cost += d * d;
  }
  
  return cost;
}

                






