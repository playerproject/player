
/**************************************************************************
 * Desc: Local controller
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
 **************************************************************************/

#ifndef ICON_H
#define ICON_H

#ifdef __cplusplus
extern "C" {
#endif

#include "inav_vector.h"
#include "inav_map.h"

// Forward declarations
struct _rtk_fig_t;


// Controller parameters
typedef struct
{
  // Parameter set id (for diagnostics)
  int id;
  
  // Weights for goal seeking and obstacle avoidance
  double kg, ko;

  // PD gain terms for each axis
  double pd[3][2];

} icon_param_t;


// Predicted path description
typedef struct
{
  // Controller settings
  icon_param_t param;
  
  // Predicted poses
  int pose_count;
  inav_vector_t poses[100];

  // Evaluated path cost
  double cost;
  
} icon_path_t;



// Controller data
typedef struct
{  
  // The local map
  imap_t *map;

  // Evaluation time interval
  double dt;

  // Pivot point
  double pivot;

  // Min obstacle distance (for collision detection)
  double min_dist;

  // Robot limits
  inav_vector_t robot_min_vel, robot_max_vel;

  // Goal pose (global cs)
  inav_vector_t goal_pose;

  // Robot pose (global cs)
  inav_vector_t robot_pose;

  // Robot velocity (robot cs)
  inav_vector_t robot_vel;

  // Current robot control velocity (robot cs)
  inav_vector_t control;

  // Predicted paths
  int path_count;
  icon_path_t paths[100];

  // Selected path
  icon_path_t *best_path;
  
} icon_t;


/**************************************************************************
 * Basic controller functions
 **************************************************************************/

// Create a new controller
icon_t *icon_alloc(imap_t *map, double min_dist);

// Destroy the controller
void icon_free(icon_t *icon);

// Set the goal pose
void icon_set_goal(icon_t *icon, double pose[3]);

// Set the current robot state: pose (map cs) and velocity (robot cs)
void icon_set_robot(icon_t *icon, double pose[3], double vel[3]);

// Compute the control velocities (robot cs)
void icon_get_control(icon_t *icon, double control[3]);


/**************************************************************************
 * GUI stuff
 **************************************************************************/

// Draw diagnostics info 
void icon_draw(icon_t *icon, struct _rtk_fig_t *fig);

#ifdef __cplusplus
}
#endif

#endif
