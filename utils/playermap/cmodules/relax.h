/*
 * Map maker
 * Copyright (C) 2003 Andrew Howard 
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/**************************************************************************
 * Desc: Graph relaxtion
 * Author: Andrew Howard
 * Date: 9 Nov 2002
 * CVS: $Id$
**************************************************************************/

#ifndef RELAX_H
#define RELAX_H

#include "geom.h"
#include "vector.h"


// Description for a single node
typedef struct
{
  // Pose of the node
  vector_t pose;

  // Free node?
  int free;

  // The node index
  int index;

} relax_node_t;


// Description for a single link
typedef struct
{
  // The nodes joined by this link.
  relax_node_t *node_a, *node_b;

  // Link type
  // 0 = point-point
  // 1 = point-line
  // 2 = line-point
  int type;

  // Link weight
  double w;

  // Outlier distance
  double outlier;
  
  // Link data: point in a lies on line in b
  geom_point_t pa, pb;
  geom_line_t la, lb;

} relax_link_t;


// Relaxation engine
typedef struct
{
  // List of nodes
  int node_count, node_max_count;
  relax_node_t **nodes;

  // List of links
  int link_count, link_max_count;
  relax_link_t **links;

} relax_t;


// Create a new relaxation engine
relax_t *relax_alloc();

// Destroy a relaxation engine
void relax_free(relax_t *relax);

// Create a new node in the graph
relax_node_t *relax_node_alloc(relax_t *relax);

// Destroy a node from the graph
void relax_node_free(relax_t *relax, relax_node_t *node);

// Create a link to the graph
relax_link_t *relax_link_alloc(relax_t *relax, relax_node_t *node_a, relax_node_t *node_b);

// Remove a link from the graph
void relax_link_free(relax_t *relax, relax_link_t *link);

// Relax the graph using least-squares
double relax_relax_ls(relax_t *relax, int steps, double epsabs, double epsrel);

// Relax the graph using non-linear minimization
double relax_relax_nl(relax_t *relax, int steps, double epsabs, double step, double tol);

#endif
