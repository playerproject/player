
/**************************************************************************
 * Desc: Local controller; RRT functions
 * Author: Andrew Howard
 * Date: 3 May 2003
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gsl/gsl_qrng.h>

#include "inav_con.h"

// Expand the tree for the given target pose
static void icon_rrt_expand(icon_t *self, inav_vector_t goal, double duration);


// Initialize the tree
void icon_rrt_init(icon_t *self)
{
  icon_node_t *node;

  // Get rid if the existing nodes
  self->node_count = 0;

  // Empty the kd tree
  inav_kdtree_clear(self->kdtree);

  // Add in the root node
  node = self->nodes + self->node_count++;
  node->config.pose = inav_vector_zero();
  node->config.vel = inav_vector_zero();
  node->parent = NULL;
  node->child_first = NULL;
  node->child_last = NULL;
  node->sibling_next = NULL;

  // Add root node to kd-tree
  inav_kdtree_insert(self->kdtree, node->config.pose, node);
    
  return;
}


// Add a node to the tree
icon_node_t *icon_rrt_add(icon_t *self, icon_node_t *node,
                          icon_config_t config, icon_action_t action)
{
  icon_node_t *nnode;

  assert(self->node_count < self->node_max_count);
  
  nnode = self->nodes + self->node_count++;
  memset(nnode, 0, sizeof(*nnode));

  nnode->action = action;
  nnode->config = config;

  nnode->parent = node;
  nnode->child_first = NULL;
  nnode->child_last = NULL;
  nnode->sibling_next = NULL;

  if (node->child_first == NULL)
    node->child_first = nnode;
  if (node->child_last != NULL)
    node->child_last->sibling_next = nnode;
  node->child_last = nnode;

  // Add  node to kd-tree
  inav_kdtree_insert(self->kdtree, nnode->config.pose, nnode);

  return nnode;
}


// Generate the tree
void icon_rrt_update(icon_t *self, int point_count, double duration)
{
  int i;
  double z[3];
  inav_vector_t goal;
  gsl_qrng *rng;

  // TESTING
  int ncount = 0;
  int mcount = 0;

  z[0] = z[1] = z[2] = 0.0;
  rng = gsl_qrng_alloc(gsl_qrng_sobol, 3);
  
  for (i = 0; i < point_count; i++)
  {
    // Uniform RNG
    //z[0] = (1.0 * rand()) / RAND_MAX;
    //z[1] = (1.0 * rand()) / RAND_MAX;
    //z[2] = 0; //(1.0 * rand()) / RAND_MAX;

    // Quasi RNG
    gsl_qrng_get(rng, z);
    
    goal.v[0] = (4.0 * z[0]) - 2.0;
    goal.v[1] = (4.0 * z[1]) - 2.0;
    goal.v[2] = (2 * M_PI * z[2]) - M_PI;

    icon_rrt_expand(self, goal, duration);

    // TESTING
    ncount += self->node_count;
    mcount += self->kdtree->mcount;
  }

  printf("%d/%d = %0.2f\n",
         ncount, mcount, (double) ncount / mcount);

  gsl_qrng_free(rng);
  
  return;
}


// Expand the tree for the given target pose
void icon_rrt_expand(icon_t *self, inav_vector_t goal, double duration)
{
  int i, j;
  inav_vector_t diff;
  double dist, min_dist;
  icon_node_t *node, *min_node;
  icon_action_t action, min_action;
  icon_config_t config, min_config;
  double m[] = {1.0, 1.0, 0.0};

  /*
  min_node = NULL;
  min_dist = DBL_MAX;

  // Find the nearest point in the tree
  for (i = 0; i < self->node_count; i++)
  {
    node = self->nodes + i;

    // Compute the metric squared distance
    diff = inav_vector_cs_sub(goal, node->config.pose);
    dist = 0;
    dist += m[0] * diff.v[0] * diff.v[0];
    dist += m[1] * diff.v[1] * diff.v[1];
    dist += m[2] * diff.v[2] * diff.v[2];

    if (dist < min_dist)
    {
      min_dist = dist;
      min_node = node;
    }
  }
  */

  // TESTING
  //inav_kdtree_insert(self->kdtree, goal, NULL);

  // Get the nearest node in the tree (exact)  
  min_node = inav_kdtree_nearest(self->kdtree, goal);

  /*
  // Compute the metric squared distance
  diff = inav_vector_cs_sub(goal, node->config.pose);
  dist = 0;
  dist += m[0] * diff.v[0] * diff.v[0];
  dist += m[1] * diff.v[1] * diff.v[1];
  dist += m[2] * diff.v[2] * diff.v[2];

  printf("%f %f : %f %d\n", min_dist, dist, dist - min_dist, node == min_node);
  //assert(node == min_node);
  */

  //min_node = node;
  
  //printf("expand: %f %f %f\n", 
  //       min_node->config.pose.v[0], min_node->config.pose.v[1], min_node->config.pose.v[2]);

  node = min_node;
  min_dist = DBL_MAX;

  // Consider all possible control actions from this node, and
  // generate a new set of nodes.  
  for (i = 0; i < self->action_count; i++)
  {
    action = self->actions[i];

    /*
    action.vel[0] *= 0.10;
    action.vel[1] *= 0.10;
    */

    action.vel[0] *= 5.00 * self->dt;
    action.vel[1] *= 10.00 * self->dt;

    action.vel[0] += node->action.vel[0];
    action.vel[1] += node->action.vel[1];

    // Clip commanded velocities
    if (action.vel[0] < 0)
      action.vel[0] = 0.0;

    //printf("      : %f %f\n", action.vel[0], action.vel[1]);

    // Compute the new configuration based on this action
    config = node->config;
    for (j = 0; j < (int) ceil(duration / self->dt); j++)
      config = icon_model_robot(self, config, action);

    // Compute the square of the distance between the new pose and the goal
    dist = inav_kdtree_dist(self->kdtree, config.pose, goal);
    if (dist < min_dist)
    {
      min_dist = dist;
      min_action = action;
      min_config = config;
    }
  }

  //printf("goal  : %f %f %f\n", goal.v[0], goal.v[1], goal.v[2]);
  //printf("action: %f %f\n", min_action.vel[0], min_action.vel[1]);
  
  // Add this new configuration to the tree
  icon_rrt_add(self, node, min_config, min_action);
  
  return;
}


#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))


// Compute the new configuration based on this action
icon_config_t icon_model_robot(icon_t *self, icon_config_t config, icon_action_t action)
{
  int i;
  //double kp;
  icon_config_t nconfig;

  nconfig = config;
  
  nconfig.vel.v[0] = action.vel[0];
  nconfig.vel.v[2] = action.vel[1];

  /*
  nconfig.pose.v[0] += self->dt * action.vel[0];
  nconfig.pose.v[1] += self->dt * action.vel[1];
  */

  /*
  // HACK
  kp = 0.05;
  
  // Compute new velocity
  nconfig.vel.v[0] += kp * (action.vel[0] - nconfig.vel.v[0]);
  nconfig.vel.v[2] += kp * (action.vel[2] - nconfig.vel.v[2]);
  */

  // Equations for differential drive
  nconfig.pose.v[0] += self->dt * nconfig.vel.v[0] * cos(nconfig.pose.v[2]);
  nconfig.pose.v[1] += self->dt * nconfig.vel.v[0] * sin(nconfig.pose.v[2]);
  nconfig.pose.v[2] += self->dt * nconfig.vel.v[2];

  // Clip the velocities
  for (i = 0; i < 3; i++)
  {
    nconfig.vel.v[i] = MIN(nconfig.vel.v[i], self->robot_max_vel.v[i]);
    nconfig.vel.v[i] = MAX(nconfig.vel.v[i], self->robot_min_vel.v[i]);
  }

  return nconfig;
}

