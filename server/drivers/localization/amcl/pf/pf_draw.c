
/**************************************************************************
 * Desc: Particle filter; drawing routines
 * Author: Andrew Howard
 * Date: 10 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>

#include "rtk.h"

#include "pf.h"
#include "pf_pdf.h"
#include "pf_kdtree.h"


// Recursively draw the kd tree
void pf_draw_tree_node(pf_t *pf, rtk_fig_t *fig, pf_kdtree_t *tree, pf_kdtree_node_t *node);

// Draw the statistics
void pf_draw_statistics(pf_t *pf, rtk_fig_t *fig);


// Draw the sample set
void pf_draw_samples(pf_t *pf, rtk_fig_t *fig, int max_samples)
{
  int i;
  double px, py;
  pf_sample_set_t *set;
  pf_sample_t *sample;

  set = pf->sets + pf->current_set;
  max_samples = MIN(max_samples, set->sample_count);
    
  for (i = 0; i < max_samples; i++)
  {
    sample = set->samples + i;

    px = sample->pose.v[0];
    py = sample->pose.v[1];

    //printf("%f %f\n", px, py);
    
    rtk_fig_point(fig, px, py);
    //rtk_fig_rectangle(fig, px, py, 0, 0.1, 0.1, 0);
  }
  
  return;
}


/* FIX

// Recursively draw the kd tree
void pf_draw_tree_node(pf_t *pf, rtk_fig_t *fig, pf_kdtree_t *tree, pf_kdtree_node_t *node)
{
  double px, py, sx, sy;
  
  if (node == NULL)
    return;
  
  sx = 0.50;
  sy = 0.50;

  px = sx * (node->key[0] + 0.5);
  py = sy * (node->key[1] + 0.5);
  
  rtk_fig_rectangle(fig, px, py, 0, sx, sy, 0);
  
  pf_draw_tree_node(pf, fig, tree, node->children[0]);
  pf_draw_tree_node(pf, fig, tree, node->children[1]);
  
  return;
}
*/


// Draw the statistics
void pf_draw_stats(pf_t *pf, rtk_fig_t *fig)
{
  int i;
  pf_vector_t mean;
  pf_matrix_t cov;
  pf_matrix_t r, d;
  double o, d1, d2;

  // Compute the distributions statistics
  pf_calc_stats(pf, &mean, &cov);

  // Compute unitary representation S = R D R^T
  pf_matrix_unitary(&r, &d, cov);

  /*
  printf("cov = \n");
  pf_matrix_fprintf(cov, stdout, "%f");
  printf("r = \n");
  pf_matrix_fprintf(r, stdout, "%f");
  printf("d = \n");
  pf_matrix_fprintf(d, stdout, "%f");
  */
  
  // Compute the orientation of the error ellipse (first eigenvector)
  o = atan2(r.m[1][0], r.m[0][0]);
  d1 = 6 * sqrt(d.m[0][0]);
  d2 = 6 * sqrt(d.m[1][1]);

  // Draw the error ellipse
  rtk_fig_ellipse(fig, mean.v[0], mean.v[1], o, d1, d2, 0);
  rtk_fig_line_ex(fig, mean.v[0], mean.v[1], o, d1);
  rtk_fig_line_ex(fig, mean.v[0], mean.v[1], o + M_PI / 2, d2);

  // Draw a direction indicator
  rtk_fig_arrow(fig, mean.v[0], mean.v[1], mean.v[2], 0.50, 0.10);
  rtk_fig_arrow(fig, mean.v[0], mean.v[1], mean.v[2] + 3 * sqrt(cov.m[2][2]), 0.50, 0.10);
  rtk_fig_arrow(fig, mean.v[0], mean.v[1], mean.v[2] - 3 * sqrt(cov.m[2][2]), 0.50, 0.10);
  
  return;
}
