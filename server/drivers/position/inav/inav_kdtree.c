
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

#include "inav_vector.h"
#include "inav_kdtree.h"

// Insert a node into the tree
static inav_kdtree_node_t *inav_kdtree_insert_node(inav_kdtree_t *self,
                                                   inav_kdtree_node_t *parent,
                                                   inav_kdtree_node_t *node,
                                                   double key[], void *value);

// Compute the squared distance between two keys
static double inav_kdtree_key_dist(inav_kdtree_t *self, double key_a[], double key_b[]);

// Recursive search for nearest node
static void inav_kdtree_search_node(inav_kdtree_t *self, inav_kdtree_node_t *node,
                                    double key[], double rdist);


// Create a tree
inav_kdtree_t *inav_kdtree_alloc(int max_size)
{
  inav_kdtree_t *self;

  self = calloc(1, sizeof(inav_kdtree_t));

  self->node_count = 0;
  self->node_max_count = max_size;
  self->nodes = calloc(self->node_max_count, sizeof(inav_kdtree_node_t));

  self->dim = 4;
  
  return self;
}


// Destroy a tree
void inav_kdtree_free(inav_kdtree_t *self)
{
  free(self->nodes);
  free(self);
  return;
}


// Clear all entries from a tree
void inav_kdtree_clear(inav_kdtree_t *self)
{
  self->root = NULL;
  self->node_count = 0;
  return;
}


// Insert a pose into the tree.
void inav_kdtree_insert(inav_kdtree_t *self, inav_vector_t pose, void *value)
{
  double key[INAV_KDTREE_MAX_DIM];

  key[0] = pose.v[0];
  key[1] = pose.v[1];
  key[2] = cos(pose.v[2]);
  key[3] = sin(pose.v[2]);
  
  self->root = inav_kdtree_insert_node(self, NULL, self->root, key, value);

  return;
}


// Insert a node into the tree
inav_kdtree_node_t *inav_kdtree_insert_node(inav_kdtree_t *self, inav_kdtree_node_t *parent,
                                            inav_kdtree_node_t *node,
                                            double key[], void *value)
{
  int i;

  // If the node doesnt exist yet...
  if (node == NULL)
  {
    assert(self->node_count < self->node_max_count);
    node = self->nodes + self->node_count++;
    memset(node, 0, sizeof(inav_kdtree_node_t));

    node->leaf = 1;
    
    if (parent == NULL)
    {
      node->depth = 0;
      for (i = 0; i < INAV_KDTREE_MAX_DIM; i++)
      {
        node->lower[i] = -10; //-DBL_MAX;
        node->upper[i] = +10; //DBL_MAX;
      }
    }
    else
    {
      node->depth = parent->depth + 1;
      for (i = 0; i < INAV_KDTREE_MAX_DIM; i++)
      {
        node->lower[i] = parent->lower[i];
        node->upper[i] = parent->upper[i];
      }
    }

    for (i = 0; i < INAV_KDTREE_MAX_DIM; i++)
      node->key[i] = key[i];
    node->value = value;
  }

  // If the node exists, but it is a leaf node...
  else if (node->leaf)
  {
    // Select a pivot dimension and value

    // Median split
    node->level = node->depth % self->dim;
    node->pivot = node->key[node->level];

    // Mean split
    //node->level = node->depth % self->dim;
    //node->pivot = (node->key[node->level] + key[node->level]) / 2;
    
    if (key[node->level] < node->pivot)
    {
      node->children[0] = inav_kdtree_insert_node(self, node, NULL, key, value);
      node->children[1] = inav_kdtree_insert_node(self, node, NULL, node->key, node->value);
    }
    else
    {
      node->children[0] = inav_kdtree_insert_node(self, node, NULL, node->key, node->value);
      node->children[1] = inav_kdtree_insert_node(self, node, NULL, key, value);
    }

    node->children[0]->upper[node->level] = node->pivot;
    node->children[1]->lower[node->level] = node->pivot;

    node->leaf = 0;
  }

  // If the node exists, and it has children...
  else
  {
    if (key[node->level] < node->pivot)
      inav_kdtree_insert_node(self, node, node->children[0], key, value);
    else
      inav_kdtree_insert_node(self, node, node->children[1], key, value);
  }
  return node;
}


// Compute the squared distance between two points
double inav_kdtree_dist(inav_kdtree_t *self, inav_vector_t pose_a, inav_vector_t pose_b)
{
  double key_a[INAV_KDTREE_MAX_DIM], key_b[INAV_KDTREE_MAX_DIM];
  
  key_a[0] = pose_a.v[0];
  key_a[1] = pose_a.v[1];
  key_a[2] = cos(pose_a.v[2]);
  key_a[3] = sin(pose_a.v[2]);

  key_b[0] = pose_b.v[0];
  key_b[1] = pose_b.v[1];
  key_b[2] = cos(pose_b.v[2]);
  key_b[3] = sin(pose_b.v[2]);

  return inav_kdtree_key_dist(self, key_a, key_b);
}


// Compute the squared distance between two keys
double inav_kdtree_key_dist(inav_kdtree_t *self, double key_a[], double key_b[])
{
  int i;
  double d;
  double m[] = {1.0, 1.0, 2.0, 2.0};
  
  d = 0;
  for (i = 0; i < self->dim; i++)
    d += (key_a[i] - key_b[i]) * m[i] * (key_a[i] - key_b[i]);

  return d;
}


// Find the approximate nearest neighbor in the tree
void *inav_kdtree_nearest(inav_kdtree_t *self, inav_vector_t pose)
{
  int i;
  double key[INAV_KDTREE_MAX_DIM];
  inav_kdtree_node_t *node;

  key[0] = pose.v[0];
  key[1] = pose.v[1];
  key[2] = cos(pose.v[2]);
  key[3] = sin(pose.v[2]);

  self->mcount = 0;
  self->mdist = DBL_MAX;
  self->mnode = NULL;
  for (i = 0; i < INAV_KDTREE_MAX_DIM; i++)
    self->moffsets[i] = 0.0;
  
  inav_kdtree_search_node(self, self->root, key, 0.0);
  node = self->mnode;
  
  assert(node != NULL);

  return node->value;
}


// Recursive search for nearest neighbor This implements the
// algorithms described in "Algorithms for Fast Vector Quantization"
// Sunil Arya, David M. Mount, 1993.
void inav_kdtree_search_node(inav_kdtree_t *self, inav_kdtree_node_t *node,
                             double key[], double rdist)
{
  double d;
  double old_offset, new_offset;

  self->mcount++;
  
  // If this is a leaf node, just look for nearest neighbor...
  if (node->leaf)
  {
    d = inav_kdtree_key_dist(self, key, node->key);
    if (d < self->mdist)
    {
      self->mdist = d;
      self->mnode = node;
    }
  }

  // If this is an internal node...
  else
  {
    old_offset = self->moffsets[node->level];
    new_offset = key[node->level] - node->pivot;

    if (new_offset < 0)
    {
      inav_kdtree_search_node(self, node->children[0], key, rdist);

      rdist += -old_offset * old_offset + new_offset * new_offset;
      if (rdist < self->mdist)
      {
        self->moffsets[node->level] = new_offset;
        inav_kdtree_search_node(self, node->children[1], key, rdist);
        self->moffsets[node->level] = old_offset;
      }
    } 
    else
    {
      inav_kdtree_search_node(self, node->children[1], key, rdist);

      rdist += -old_offset * old_offset + new_offset * new_offset;
      if (rdist < self->mdist)
      {
        self->moffsets[node->level] = new_offset;
        inav_kdtree_search_node(self, node->children[0], key, rdist);
        self->moffsets[node->level] = old_offset;
      }
    }
  }
  return;
}




/*
// Find the approximate nearest neighbor in the tree
void *inav_kdtree_nearest(inav_kdtree_t *self, inav_vector_t pose)
{
  double key[3];
  inav_kdtree_node_t *node;

  key[0] = pose.v[0];
  key[1] = pose.v[1];
  key[2] = pose.v[2];

  node = inav_kdtree_find_node(self, self->root, key);

  return node->value;
}

// Find a node into the tree (the place the point would get inserted)
inav_kdtree_node_t *inav_kdtree_find_node(inav_kdtree_t *self,
                                          inav_kdtree_node_t *node,
                                          double key[])
{
  if (node->leaf)
  {
    return node;
  }
  else
  {
    if (key[node->level] < node->pivot)
      return inav_kdtree_find_node(self, node->children[0], key);
    else
      return inav_kdtree_find_node(self, node->children[1], key);
  }

  // Can never get here
  assert(0);
  return NULL;
}
*/



#ifdef INCLUDE_RTKGUI

// Recursively draw nodes
static void inav_kdtree_draw_node(inav_kdtree_t *self, inav_kdtree_node_t *node, rtk_fig_t *fig);


// Draw the tree
void inav_kdtree_draw(inav_kdtree_t *self, rtk_fig_t *fig)
{
  inav_kdtree_draw_node(self, self->root, fig);
  return;
}


// Recursively draw nodes
void inav_kdtree_draw_node(inav_kdtree_t *self, inav_kdtree_node_t *node, rtk_fig_t *fig)
{
  //char text[64];
  
  if (node->leaf)
  {
    rtk_fig_ellipse(fig, node->key[0], node->key[1], 0, 0.01, 0.01, 1);

    //snprintf(text, sizeof(text), "%d", node->depth);
    //rtk_fig_text(fig, node->key[0] + 0.05, node->key[1] + 0.05, 0, text);

    rtk_fig_rectangle(fig,
                      (+node->lower[0] + node->upper[0]) / 2,
                      (+node->lower[1] + node->upper[1]) / 2, 0,
                      (-node->lower[0] + node->upper[0]),
                      (-node->lower[1] + node->upper[1]), 0);
  }
  else
  {
    assert(node->children[0] != NULL);
    assert(node->children[1] != NULL);
    inav_kdtree_draw_node(self, node->children[0], fig);
    inav_kdtree_draw_node(self, node->children[1], fig);
  }
  
  return;
}

#endif
