
/**************************************************************************
 * Desc: KD tree functions
 * Author: Andrew Howard
 * Date: 18 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef PF_KDTREE_H
#define PF_KDTREE_H


#ifdef INCLUDE_RTKGUI
#include "rtk.h"
#endif


// Info for a node in the tree
typedef struct pf_kdtree_node
{
  // Depth in the tree
  int leaf, depth;

  // Pivot dimension and value
  int pivot_dim;
  int pivot_value;
  
  // The key for this node
  int key[3];

  // The value for this node
  double value;

  // Child nodes
  struct pf_kdtree_node *children[2];

} pf_kdtree_node_t;


// A kd tree
typedef struct
{
  // Cell size
  double size[3];
  
  // The root node of the tree
  pf_kdtree_node_t *root;

  // The number of nodes in the tree
  int node_count, node_max_count;
  pf_kdtree_node_t *nodes;

  // The number of leaf nodes in the tree
  int leaf_count;

} pf_kdtree_t;


// Create a tree
pf_kdtree_t *pf_kdtree_alloc(int max_size);

// Destroy a tree
void pf_kdtree_free(pf_kdtree_t *self);

// Clear all entries from the tree
void pf_kdtree_clear(pf_kdtree_t *self);

// Insert a pose into the tree
void pf_kdtree_insert(pf_kdtree_t *tree, pf_vector_t pose, double value);

// Determine the probability estimate for the given pose
double pf_kdtree_prob(pf_kdtree_t *tree, pf_vector_t pose);

#ifdef INCLUDE_RTKGUI

// Draw the tree
void pf_kdtree_draw(pf_kdtree_t *self, rtk_fig_t *fig);

#endif

#endif
