
/**************************************************************************
 * Desc: KD tree functions
 * Author: Andrew Howard
 * Date: 18 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef INAV_KDTREE_H
#define INAV_KDTREE_H

#ifdef INCLUDE_RTKGUI
#include "rtk.h"
#endif

#define INAV_KDTREE_MAX_DIM 4

// Info for a node in the tree
typedef struct inav_kdtree_node
{
  // Depth in the tree
  int leaf, depth, level;

  // The pivot value
  double pivot;

  // Bounding box
  double lower[INAV_KDTREE_MAX_DIM], upper[INAV_KDTREE_MAX_DIM];

  // Child nodes
  struct inav_kdtree_node *children[2];

  // Key associated with this node
  double key[INAV_KDTREE_MAX_DIM];
  
  // Value associated with this node
  void *value;

} inav_kdtree_node_t;


// A kd tree
typedef struct
{
  // The dimension of the key
  int dim;
  
  // The root node of the tree
  inav_kdtree_node_t *root;

  // The node lest
  int node_count, node_max_count;
  inav_kdtree_node_t *nodes;

  // Values used for nearest neighbor search
  int mcount;
  inav_kdtree_node_t *mnode;
  double mdist;
  double moffsets[INAV_KDTREE_MAX_DIM];

} inav_kdtree_t;



// Create a tree
inav_kdtree_t *inav_kdtree_alloc(int max_size);

// Destroy a tree
void inav_kdtree_free(inav_kdtree_t *self);

// Clear all entries from a tree
void inav_kdtree_clear(inav_kdtree_t *self);

// Insert a pose into the tree
void inav_kdtree_insert(inav_kdtree_t *self, inav_vector_t pose, void *value);

// Find the approximate nearest neighbor in the tree
void *inav_kdtree_nearest(inav_kdtree_t *self, inav_vector_t pose);

// Compute the squared distance between two points
double inav_kdtree_dist(inav_kdtree_t *self, inav_vector_t pose_a, inav_vector_t pose_b);

#ifdef INCLUDE_RTKGUI

// Draw the tree
void inav_kdtree_draw(inav_kdtree_t *self, rtk_fig_t *fig);

#endif

#endif
