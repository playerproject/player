
/**************************************************************************
 * Desc: KD tree functions
 * Author: Andrew Howard
 * Date: 18 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef PF_KDTREE_H
#define PF_KDTREE_H



// Forward declarations
struct pf_kdtree_node;


// Info for a node in the tree
typedef struct pf_kdtree_node
{
  // The key for this node
  int key[3];

  // Level in the tree
  int level;
  
  // Child nodes
  struct pf_kdtree_node *children[2];

  // The number of hits to this node
  int count;

} pf_kdtree_node_t;


// A kd tree
typedef struct
{  
  // The root node of the tree
  pf_kdtree_node_t *root;

  // The number of nodes in the tree
  int node_count;

} pf_kdtree_t;



// Create a tree
pf_kdtree_t *pf_kdtree_alloc();

// Destroy a tree
void pf_kdtree_free(pf_kdtree_t *tree);

// Insert a pose into the tree
void pf_kdtree_insert(pf_kdtree_t *tree, pf_vector_t pose);

// Determine the probability estimate for the given pose
double pf_kdtree_prob(pf_kdtree_t *tree, pf_vector_t pose);


#endif
