
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
#include "inav_kdtree.h"
#include "inav_map.h"

// Forward declarations
struct _rtk_fig_t;

// Robot configuration
typedef struct
{
  // Robot pose and velocity
  inav_vector_t pose, vel;
  
} icon_config_t;


// Robot actions
typedef struct
{
  // Commanded velocities
  double vel[2];
  
} icon_action_t;


// A single node in the tree
typedef struct _icon_node_t
{
  // Index of parent, first child, and next sibling
  struct _icon_node_t *parent;
  struct _icon_node_t *sibling_next;
  struct _icon_node_t *child_first, *child_last;

  // Robot configuration
  icon_config_t config;

  // Action that will get the robot to this configuration
  icon_action_t action;
  
} icon_node_t;
  


// Controller data
typedef struct
{  
  // The local map
  imap_t *map;

  // Evaluation time interval
  double dt;

  // Min obstacle distance (for collision detection)
  double min_dist;

  // Control limits
  inav_vector_t robot_min_vel, robot_max_vel;

  // Possible actions
  int action_count;
  icon_action_t *actions;

  // Goal pose (global cs)
  inav_vector_t goal_pose;

  // Robot pose (global cs)
  inav_vector_t robot_pose;

  // Robot velocity (robot cs)
  inav_vector_t robot_vel;

  // The plan tree (RRT)
  int node_count, node_max_count;
  icon_node_t *nodes;

  // A kd-tree representation of the plan tree
  inav_kdtree_t *kdtree;
  
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

// Compute the new configuration based on this action
icon_config_t icon_model_robot(icon_t *self, icon_config_t config, icon_action_t action);

// Initialize the tree
void icon_rrt_init(icon_t *self);

// Generate the tree
void icon_rrt_update(icon_t *self, int point_count, double duration);


/**************************************************************************
 * GUI stuff
 **************************************************************************/

// Draw diagnostics info 
void icon_draw(icon_t *icon, struct _rtk_fig_t *fig);

#ifdef __cplusplus
}
#endif

#endif
