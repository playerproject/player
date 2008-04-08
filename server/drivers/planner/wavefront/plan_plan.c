
/**************************************************************************
 * Desc: Path planner: plan generation
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "plan.h"

// Plan queue stuff
void plan_push(plan_t *plan, plan_cell_t *cell);
plan_cell_t *plan_pop(plan_t *plan);
int _plan_update_plan(plan_t *plan, double lx, double ly, double gx, double gy);
int _plan_find_local_goal(plan_t *plan, double* gx, double* gy, double lx, double ly);


int
plan_do_global(plan_t *plan, double lx, double ly, double gx, double gy)
{
  plan_cell_t* cell;
  int li, lj;

  // Set bounds to look over the entire grid
  plan_set_bounds(plan, 0, 0, plan->size_x - 1, plan->size_y - 1);

  // Reset plan costs
  plan_reset(plan);

  plan->path_count = 0;
  if(_plan_update_plan(plan, lx, ly, gx, gy) != 0)
  {
    // no path
    return(-1);
  }

  li = PLAN_GXWX(plan, lx);
  lj = PLAN_GYWY(plan, ly);

  // Cache the path
  for(cell = plan->cells + PLAN_INDEX(plan,li,lj);
      cell;
      cell = cell->plan_next)
  {
    if(plan->path_count >= plan->path_size)
    {
      plan->path_size *= 2;
      plan->path = (plan_cell_t**)realloc(plan->path,
                                          plan->path_size * sizeof(plan_cell_t*));
      assert(plan->path);
    }
    plan->path[plan->path_count++] = cell;
  }

  return(0);
}

int
plan_do_local(plan_t *plan, double lx, double ly, double plan_halfwidth)
{
  double gx, gy;

  // Set bounds as directed
  plan_set_bounds(plan, 
                  lx - plan_halfwidth, ly - plan_halfwidth,
                  lx + plan_halfwidth, ly + plan_halfwidth);

  // Reset plan costs (within the local patch)
  plan_reset(plan);

  // Find a local goal to pursue
  if(_plan_find_local_goal(plan, &gx, &gy, lx, ly) != 0)
    return(-1);

  return(_plan_update_plan(plan, lx, ly, gx, gy));
}


// Generate the plan
int 
_plan_update_plan(plan_t *plan, double lx, double ly, double gx, double gy)
{
  int oi, oj, di, dj, ni, nj;
  int gi, gj, li,lj;
  float cost;
  plan_cell_t *cell, *ncell;

  // Reset the grid
  cell = plan->cells;
  for (nj = 0; nj < plan->size_y; nj++)
  {
    for (ni = 0; ni < plan->size_x; ni++,cell++)
    {
      cell->plan_cost = PLAN_MAX_COST;
      cell->plan_next = NULL;
      cell->mark = 0;
    }
  }
  
  // Reset the queue
  heap_reset(plan->heap);

  // Initialize the goal cell
  gi = PLAN_GXWX(plan, gx);
  gj = PLAN_GYWY(plan, gy);
  
  if (!PLAN_VALID_BOUNDS(plan, gi, gj))
    return(-1);
  
  // Initialize the start cell
  li = PLAN_GXWX(plan, lx);
  lj = PLAN_GYWY(plan, ly);
  
  if (!PLAN_VALID_BOUNDS(plan, li, lj))
    return(-1);

  cell = plan->cells + PLAN_INDEX(plan, gi, gj);
  cell->plan_cost = 0;
  plan_push(plan, cell);

  while (1)
  {
    cell = plan_pop(plan);
    if (cell == NULL)
      break;

    oi = cell->ci;
    oj = cell->cj;

    //printf("pop %d %d %f\n", cell->ci, cell->cj, cell->plan_cost);

    float* p = plan->dist_kernel_3x3;
    for (dj = -1; dj <= +1; dj++)
    {
      ncell = plan->cells + PLAN_INDEX(plan,oi-1,oj+dj);
      for (di = -1; di <= +1; di++, p++, ncell++)
      {
        if (!di && !dj)
          continue;
        //if (di && dj)
          //continue;
        
        ni = oi + di;
        nj = oj + dj;

        if (!PLAN_VALID_BOUNDS(plan, ni, nj))
          continue;

        if(ncell->mark)
          continue;

        if (ncell->occ_dist_dyn < plan->abs_min_radius)
          continue;

        cost = cell->plan_cost + *p;

        if(ncell->occ_dist_dyn < plan->max_radius)
          cost += plan->dist_penalty * (plan->max_radius - ncell->occ_dist_dyn);

        if(cost < ncell->plan_cost)
        {
          ncell->plan_cost = cost;
          ncell->plan_next = cell;

          // Are we done?
          if((ncell->ci == li) && (ncell->cj == lj))
            return(0);

          plan_push(plan, ncell);
        }
      }
    }
  }

  return(-1);
}

int 
_plan_find_local_goal(plan_t *plan, double* gx, double* gy, 
                      double lx, double ly)
{
  return(0);
}

// Push a plan location onto the queue
void plan_push(plan_t *plan, plan_cell_t *cell)
{
  // Substract from max cost because the heap is set up to return the max
  // element.  This could of course be changed.
  assert(PLAN_MAX_COST-cell->plan_cost > 0);
  cell->mark = 1;
  heap_insert(plan->heap, PLAN_MAX_COST - cell->plan_cost, cell);

  return;
}


// Pop a plan location from the queue
plan_cell_t *plan_pop(plan_t *plan)
{

  if(heap_empty(plan->heap))
    return(NULL);
  else
    return(heap_extract_max(plan->heap));
}
