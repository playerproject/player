
/**************************************************************************
 * Desc: kd-tree functions
 * Author: Andrew Howard
 * Date: 18 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "pf_vector.h"
#include "pf_kdtree.h"


// Insert a node into the tree
pf_kdtree_node_t * pf_kdtree_insert_node(pf_kdtree_t *tree, pf_kdtree_node_t *node,
                                         int depth, int key[]);

// Delete a node from the tree
void pf_kdtree_delete_node(pf_kdtree_t *tree, pf_kdtree_node_t *node);

// Recursive node search
pf_kdtree_node_t *pf_kdtree_find_node(pf_kdtree_t *tree, pf_kdtree_node_t *node,
                                      int depth, int key[]);


// Create a tree
pf_kdtree_t *pf_kdtree_alloc()
{
  pf_kdtree_t *tree;

  tree = calloc(1, sizeof(pf_kdtree_t));
  
  return tree;
}


// Destroy a tree
void pf_kdtree_free(pf_kdtree_t *tree)
{
  pf_kdtree_delete_node(tree, tree->root);
  free(tree);
  return;
}


// Insert a pose into the tree.
void pf_kdtree_insert(pf_kdtree_t *tree, pf_vector_t pose)
{
  int key[3];

  // TESTING
  key[0] = floor(pose.v[0] / 0.50);
  key[1] = floor(pose.v[1] / 0.50);
  key[2] = floor(pose.v[2] / (10 * M_PI / 180));

  tree->root = pf_kdtree_insert_node(tree, tree->root, 0, key);
  return;
}


// Find the node containing the given pose.
pf_kdtree_node_t *pf_kdtree_find(pf_kdtree_t *tree, pf_vector_t pose)
{
  int key[3];

  // TESTING
  key[0] = floor(pose.v[0] / 0.50);
  key[1] = floor(pose.v[1] / 0.50);
  key[2] = floor(pose.v[2] / (10 * M_PI / 180));
  
  return pf_kdtree_find_node(tree, tree->root, 0, key);
}


// Insert a node into the tree
pf_kdtree_node_t *pf_kdtree_insert_node(pf_kdtree_t *tree, pf_kdtree_node_t *node,
                                        int depth, int key[])
{
  int level;
  
  if (node == NULL)
  {
    node = calloc(1, sizeof(pf_kdtree_node_t));
    memcpy(node->key, key, sizeof(node->key));
    node->count = 1;
    tree->node_count++;
  }
  else
  {
    //printf("(%d %d %d) (%d %d %d)",
    //       key[0], key[1], key[2], node->key[0], node->key[1], node->key[2]);

    // Compare keys to see if they identical
    for (level = 0; level < 3 && key[level] == node->key[level]; level++);

    // If the keys are the same...
    if (level == 3)
    {
      node->count++;
    }

    // If the keys are different...
    else
    {
      level = depth % 3;
      if (key[level] < node->key[level])
        node->children[0] = pf_kdtree_insert_node(tree, node->children[0], depth + 1, key);
      else
        node->children[1] = pf_kdtree_insert_node(tree, node->children[1], depth + 1, key);
    }
  }
  return node;
}


// Delete a node from the tree
void pf_kdtree_delete_node(pf_kdtree_t *tree, pf_kdtree_node_t *node)
{
  if (node == NULL)
    return;

  pf_kdtree_delete_node(tree, node->children[0]);
  pf_kdtree_delete_node(tree, node->children[1]);  
  tree->node_count--;
  free(node);

  return;
}


// Recursive node search
pf_kdtree_node_t *pf_kdtree_find_node(pf_kdtree_t *tree, pf_kdtree_node_t *node,
                                      int depth, int key[])
{
  int level;
  
  if (node == NULL)
    return NULL;

  // Compare keys to see if they identical
  for (level = 0; level < 3 && key[level] == node->key[level]; level++);

  // If the keys are the same...
  if (level == 3)
      return node;

  // If the keys are different...
  level = depth % 3;
  if (key[level] < node->key[level])
    return pf_kdtree_find_node(tree, node->children[0], depth + 1, key);
  else
    return pf_kdtree_find_node(tree, node->children[1], depth + 1, key);

  return NULL;
}


// Determine the probability estimate for the given pose. TODO: this
// should do a kernel density estimate rather than a simple histogram.
double pf_kdtree_prob(pf_kdtree_t *tree, pf_vector_t pose)
{
  double p;
  pf_kdtree_node_t *node;

  node = pf_kdtree_find(tree, pose);
  if (node == NULL)
    return 0.0;

  // HACK; FIX: assumes all samples have equal weight, and the
  // weights are normalized.
  p = node->count;

  return p;
}

