
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


// Create a new controller
icon_t *icon_alloc(imap_t *map, double min_dist)
{
  icon_t *self;
  int i;

  self = (icon_t*) calloc(1, sizeof(icon_t));

  self->map = map;
  self->dt = 0.1;
  self->min_dist = min_dist;
  
  self->robot_min_vel.v[0] = -0.10;
  self->robot_min_vel.v[2] = -30 * M_PI / 180;
  self->robot_max_vel.v[0] = +0.50;
  self->robot_max_vel.v[2] = +30 * M_PI / 180;

  self->action_count = 9;
  self->actions = calloc(self->action_count, sizeof(self->actions[0]));
  for (i = 0; i < 9; i++)
  {
    self->actions[i].vel[0] = (i / 3) - 1;
    self->actions[i].vel[1] = (i % 3) - 1;
  }
  
  self->node_count = 0;
  self->node_max_count = 20000;
  self->nodes = calloc(self->node_max_count, sizeof(self->nodes[0]));

  self->kdtree = inav_kdtree_alloc(30000);
  
  return self;
}


// Destroy the controller
void icon_free(icon_t *self)
{
  inav_kdtree_free(self->kdtree);
  free(self->nodes);
  free(self);
  return;
}


// Set the goal pose
void icon_set_goal(icon_t *self, double pose[3])
{
  self->goal_pose.v[0] = pose[0];
  self->goal_pose.v[1] = pose[1];
  self->goal_pose.v[2] = pose[2];

  return;
}


// Set the current robot state: pose (relative to the map) and
// velocity.
void icon_set_robot(icon_t *self, double pose[3], double vel[3])
{
  self->robot_pose.v[0] = pose[0];
  self->robot_pose.v[1] = pose[1];
  self->robot_pose.v[2] = pose[2];
  self->robot_vel.v[0] = vel[0];
  self->robot_vel.v[1] = vel[1];
  self->robot_vel.v[2] = vel[2];

  return;
}


// Compute the control vector
void icon_get_control(icon_t *self, double control[3])
{
  // TODO

  icon_rrt_init(self);

  icon_rrt_update(self, 10000, 0.2);
  //icon_rrt_update(self, 200, 0.5);
  //icon_rrt_update(self, 400, 0.1);
  
  return;
}


