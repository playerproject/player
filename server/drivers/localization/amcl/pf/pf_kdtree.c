
/**************************************************************************
 * Desc: kd-tree functions
 * Author: Andrew Howard
 * Date: 18 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "pf_vector.h"
#include "pf_kdtree.h"



// Compare keys to see if they are equal
static int pf_kdtree_equal(pf_kdtree_t *self, int key_a[], int key_b[]);

// Insert a node into the tree
static pf_kdtree_node_t *pf_kdtree_insert_node(pf_kdtree_t *self, pf_kdtree_node_t *parent,
                                               pf_kdtree_node_t *node, int key[], double value);

// Recursive node search
static pf_kdtree_node_t *pf_kdtree_find_node(pf_kdtree_t *self, pf_kdtree_node_t *node, int key[]);


// Create a tree
pf_kdtree_t *pf_kdtree_alloc(int max_size)
{
  pf_kdtree_t *self;

  self = calloc(1, sizeof(pf_kdtree_t));

  self->size[0] = 0.50;
  self->size[1] = 0.50;
  self->size[2] = (10 * M_PI / 180);

  self->root = NULL;
    
  self->node_count = 0;
  self->node_max_count = max_size;
  self->nodes = calloc(self->node_max_count, sizeof(pf_kdtree_node_t));

  self->leaf_count = 0;
  
  return self;
}


// Destroy a tree
void pf_kdtree_free(pf_kdtree_t *self)
{
  free(self->nodes);
  free(self);
  return;
}

// Clear all entries from the tree
void pf_kdtree_clear(pf_kdtree_t *self)
{
  self->root = NULL;
  self->leaf_count = 0;
  self->node_count = 0;
  
  return;
}


// Insert a pose into the tree.
void pf_kdtree_insert(pf_kdtree_t *self, pf_vector_t pose, double value)
{
  int key[3];

  key[0] = floor(pose.v[0] / self->size[0]);
  key[1] = floor(pose.v[1] / self->size[1]);
  key[2] = floor(pose.v[2] / self->size[2]);

  self->root = pf_kdtree_insert_node(self, NULL, self->root, key, value);
  return;
}


// Determine the probability estimate for the given pose. TODO: this
// should do a kernel density estimate rather than a simple histogram.
double pf_kdtree_prob(pf_kdtree_t *self, pf_vector_t pose)
{
  int key[3];
  pf_kdtree_node_t *node;

  key[0] = floor(pose.v[0] / self->size[0]);
  key[1] = floor(pose.v[1] / self->size[1]);
  key[2] = floor(pose.v[2] / self->size[2]);
  
  node = pf_kdtree_find_node(self, self->root, key);
  if (node == NULL)
    return 0.0;
  return node->value;
}


// Compare keys to see if they are equal
int pf_kdtree_equal(pf_kdtree_t *self, int key_a[], int key_b[])
{
  int i;

  for (i = 0; i < 3; i++)
    if (key_a[i] != key_b[i])
      break;

  return (i == 3);
}


// Insert a node into the tree
pf_kdtree_node_t *pf_kdtree_insert_node(pf_kdtree_t *self, pf_kdtree_node_t *parent,
                                        pf_kdtree_node_t *node, int key[], double value)
{
  int i;
  
  // If the node doesnt exist yet...
  if (node == NULL)
  {
    assert(self->node_count < self->node_max_count);
    node = self->nodes + self->node_count++;
    memset(node, 0, sizeof(pf_kdtree_node_t));

    node->leaf = 1;
    
    if (parent == NULL)
      node->depth = 0;
    else
      node->depth = parent->depth + 1;

    for (i = 0; i < 3; i++)
      node->key[i] = key[i];
    
    node->value = value;
    self->leaf_count += 1;
  }

  // If the node exists, and it is a leaf node...
  else if (node->leaf)
  {
    // If the keys are equal, increment the value
    if (pf_kdtree_equal(self, key, node->key))
    {
      node->value += value;
    }

    // The keys are not equal, so split this node
    else
    {
      // Median split
      node->pivot_dim = node->depth % 3;
      node->pivot_value = node->key[node->pivot_dim];
    
      if (key[node->pivot_dim] < node->pivot_value)
      {
        node->children[0] = pf_kdtree_insert_node(self, node, NULL, key, value);
        node->children[1] = pf_kdtree_insert_node(self, node, NULL, node->key, node->value);
      }
      else
      {
        node->children[0] = pf_kdtree_insert_node(self, node, NULL, node->key, node->value);
        node->children[1] = pf_kdtree_insert_node(self, node, NULL, key, value);
      }
      
      node->leaf = 0;
      self->leaf_count -= 1;
    }
  }

  // If the node exists, and it has children...
  else
  {
    assert(node->children[0] != NULL);
    assert(node->children[1] != NULL);

    if (key[node->pivot_dim] < node->pivot_value)
      pf_kdtree_insert_node(self, node, node->children[0], key, value);
    else
      pf_kdtree_insert_node(self, node, node->children[1], key, value);
  }

  return node;
}


// Recursive node search
pf_kdtree_node_t *pf_kdtree_find_node(pf_kdtree_t *self, pf_kdtree_node_t *node, int key[])
{
  if (node->leaf)
  {
    // If the keys are the same...
    if (pf_kdtree_equal(self, key, node->key))
      return node;
    else
      return NULL;
  }
  else
  {
    assert(node->children[0] != NULL);
    assert(node->children[1] != NULL);

    // If the keys are different...
    if (key[node->pivot_dim] < node->key[node->pivot_dim])
      return pf_kdtree_find_node(self, node->children[0], key);
    else
      return pf_kdtree_find_node(self, node->children[1], key);
  }

  return NULL;
}


#ifdef INCLUDE_RTKGUI

// Recursively draw nodes
static void pf_kdtree_draw_node(pf_kdtree_t *self, pf_kdtree_node_t *node, rtk_fig_t *fig);


// Draw the tree
void pf_kdtree_draw(pf_kdtree_t *self, rtk_fig_t *fig)
{
  if (self->root != NULL)
    pf_kdtree_draw_node(self, self->root, fig);
  return;
}


// Recursively draw nodes
void pf_kdtree_draw_node(pf_kdtree_t *self, pf_kdtree_node_t *node, rtk_fig_t *fig)
{
  double ox, oy;
  //char text[64];
  
  if (node->leaf)
  {
    ox = (node->key[0] + 0.5) * self->size[0];
    oy = (node->key[1] + 0.5) * self->size[1];
    
    rtk_fig_rectangle(fig, ox, oy, 0.0, self->size[0], self->size[1], 0);

    //snprintf(text, sizeof(text), "%0.3f", node->value);
    //rtk_fig_text(fig, ox, oy, 0.0, text);
  }
  else
  {
    assert(node->children[0] != NULL);
    assert(node->children[1] != NULL);
    pf_kdtree_draw_node(self, node->children[0], fig);
    pf_kdtree_draw_node(self, node->children[1], fig);
  }
  
  return;
}

#endif
