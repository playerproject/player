
/**************************************************************************
 * Desc: Path planner: waypoint generation
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "plan.h"

// Test to see if once cell is reachable from another
int plan_test_reachable(plan_t *plan, plan_cell_t *cell_a, plan_cell_t *cell_b);


// Generate a path to the goal
void plan_update_waypoints(plan_t *plan, double px, double py)
{
  double dist;
  int ni, nj;
  plan_cell_t *cell, *ncell;
  
  ni = PLAN_GXWX(plan, px);
  nj = PLAN_GYWY(plan, py);
  cell = plan->cells + PLAN_INDEX(plan, ni, nj);

  plan->waypoint_count = 0;
  
  while (cell != NULL)
  {
    //printf("%d %d\n", cell->ci, cell->cj);

    if (plan->waypoint_count >= plan->waypoint_size)
    {
      plan->waypoint_size *= 2;
      plan->waypoints = realloc(plan->waypoints,
                                plan->waypoint_size * sizeof(plan->waypoints[0]));
    }
    
    plan->waypoints[plan->waypoint_count++] = cell;

    if (cell->plan_next == NULL)
    {
      // done
      break;
    }

    // Find the farthest cell in the path that is reachable from the
    // currrent cell.
    dist = 0;
    for (ncell = cell; ncell->plan_next != NULL; ncell = ncell->plan_next)
    {
      if (dist > 0.50)
        if (!plan_test_reachable(plan, cell, ncell->plan_next))
          break;
      dist += plan->scale;
    }
    if(ncell == cell)
    {
      break;
    }
    
    cell = ncell;
  }

  if(cell && (cell->plan_cost > 0))
  {
    // no path
    plan->waypoint_count = 0;
  }
  
  return;
}


// Get the ith waypoint; returns non-zero of there are no more waypoints
int plan_get_waypoint(plan_t *plan, int i, double *px, double *py)
{
  if (i < 0 || i >= plan->waypoint_count)
    return 0;

  *px = PLAN_WXGX(plan, plan->waypoints[i]->ci);
  *py = PLAN_WYGY(plan, plan->waypoints[i]->cj);

  return 1;
}


// Test to see if once cell is reachable from another.
// This could be improved.
int plan_test_reachable(plan_t *plan, plan_cell_t *cell_a, plan_cell_t *cell_b)
{
  int i, j;
  int ai, aj, bi, bj;
  double ox, oy, oa;
  double dx, dy;
  plan_cell_t *cell;

  ai = cell_a->ci;
  aj = cell_a->cj;
  bi = cell_b->ci;
  bj = cell_b->cj;

  ox = PLAN_WXGX(plan, ai);
  oy = PLAN_WYGY(plan, aj);
  oa = atan2(bj - aj, bi - ai);
  
  if (fabs(cos(oa)) > fabs(sin(oa)))
  {
    dy = tan(oa) * plan->scale;

    if (ai < bi)
    {
      for (i = ai; i < bi; i++)
      {
        j = PLAN_GYWY(plan, oy + (i - ai) * dy);
        if (PLAN_VALID(plan, i, j))
        {
          cell = plan->cells + PLAN_INDEX(plan, i, j);
          if (cell->occ_dist < plan->abs_min_radius)
            return 0;
        }
      }
    }
    else
    {
      for (i = ai; i > bi; i--)
      {
        j = PLAN_GYWY(plan, oy + (i - ai) * dy);
        if (PLAN_VALID(plan, i, j))
        {
          cell = plan->cells + PLAN_INDEX(plan, i, j);
          if (cell->occ_dist < plan->abs_min_radius)
            return 0;
        }
      }
    }
  }
  else
  {
    dx = tan(M_PI/2 - oa) * plan->scale;

    if (aj < bj)
    {
      for (j = aj; j < bj; j++)
      {
        i = PLAN_GXWX(plan, ox + (j - aj) * dx);
        if (PLAN_VALID(plan, i, j))
        {
          cell = plan->cells + PLAN_INDEX(plan, i, j);
          if (cell->occ_dist < plan->abs_min_radius)
            return 0;
        }
      }
    }
    else
    {
      for (j = aj; j > bj; j--)
      {
        i = PLAN_GXWX(plan, ox + (j - aj) * dx);
        if (PLAN_VALID(plan, i, j))
        {
          cell = plan->cells + PLAN_INDEX(plan, i, j);
          if (cell->occ_dist < plan->abs_min_radius)
            return 0;
        }
      }
    }
  }
  return 1;
}

