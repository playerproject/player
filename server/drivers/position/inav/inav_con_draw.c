
/**************************************************************************
 * Desc: Local controller; GUI stuff
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <rtk.h>

#include "inav_con.h"

// Draw the tree
static void icon_draw_tree(icon_t *self, rtk_fig_t *fig);

// Draw the tree recursively
static void icon_draw_tree_node(icon_t *self, icon_node_t *node, rtk_fig_t *fig);



// Draw diagnostics
void icon_draw(icon_t *self, rtk_fig_t *fig)
{
  inav_vector_t pose_a;

  // Draw the goal
  pose_a = self->goal_pose;
  rtk_fig_ellipse(fig, pose_a.v[0], pose_a.v[1], pose_a.v[2], 0.20, 0.20, 0);

  // Draw the tree
  icon_draw_tree(self, fig);

  return;
}


// Draw the tree
void icon_draw_tree(icon_t *self, rtk_fig_t *fig)
{
  if (self->node_count == 0)
    return;

  rtk_fig_color(fig, 1.0, 0.0, 0.0);
  icon_draw_tree_node(self, self->nodes, fig);

  // Draw the kdtree
  //rtk_fig_color(fig, 0.0, 0.0, 1.0);
  //inav_kdtree_draw(self->kdtree, fig);

  return;
}


// Draw the tree recursively
void icon_draw_tree_node(icon_t *self, icon_node_t *node, rtk_fig_t *fig)
{
  icon_node_t *nnode;
  inav_vector_t pose_a, pose_b;

  pose_a = node->config.pose;
  //rtk_fig_rectangle(fig, pose_a.v[0], pose_a.v[1], pose_a.v[2], 0.10, 0.05, 0);

  for (nnode = node->child_first; nnode != NULL; nnode = nnode->sibling_next)
  {
    pose_b = nnode->config.pose;
    rtk_fig_line(fig, pose_a.v[0], pose_a.v[1], pose_b.v[0], pose_b.v[1]);

    icon_draw_tree_node(self, nnode, fig);
  }

  return;
}
