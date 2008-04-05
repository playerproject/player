
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


// Generate the plan
void 
plan_update_plan(plan_t *plan, double lx, double ly, double gx, double gy)
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
    }
  }
  
  // Reset the queue
  //plan->queue_start = 0;
  //plan->queue_len = 0;
  heap_reset(plan->heap);

  // Initialize the goal cell
  gi = PLAN_GXWX(plan, gx);
  gj = PLAN_GYWY(plan, gy);
  
  if (!PLAN_VALID_BOUNDS(plan, gi, gj))
    return;
  
  // Initialize the start cell
  li = PLAN_GXWX(plan, lx);
  lj = PLAN_GYWY(plan, ly);
  
  if (!PLAN_VALID_BOUNDS(plan, li, lj))
    return;

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

    for (dj = -1; dj <= +1; dj++)
    {
      for (di = -1; di <= +1; di++)
      {
        if (!di && !dj)
          continue;
        //if (di && dj)
          //continue;
        
        ni = oi + di;
        nj = oj + dj;

        if (!PLAN_VALID_BOUNDS(plan, ni, nj))
          continue;
        ncell = plan->cells + PLAN_INDEX(plan, ni, nj);

        if (ncell->occ_dist < plan->abs_min_radius)
          continue;

        /*
        if(di && dj)
          cost = cell->plan_cost + plan->scale * M_SQRT2;
        else
          cost = cell->plan_cost + plan->scale;
          */

        cost = cell->plan_cost + plan->dist_kernel_3x3[di+1][dj+1];

        if (ncell->occ_dist < plan->max_radius)
          cost += plan->dist_penalty * (plan->max_radius - ncell->occ_dist);

        if (cost < ncell->plan_cost)
        {
          ncell->plan_cost = cost;
          ncell->plan_next = cell;

          // Are we done?
          if((ncell->ci == li) && (ncell->cj == lj))
            return;
          plan_push(plan, ncell);
        }
      }
    }
  }

  return;
}


// Push a plan location onto the queue
void plan_push(plan_t *plan, plan_cell_t *cell)
{
  /*
  int i;

  // HACK: should resize the queue dynamically (tricky for circular queue)
  assert(plan->queue_len < plan->queue_size);

  i = (plan->queue_start + plan->queue_len) % plan->queue_size;
  plan->queue[i] = cell;
  plan->queue_len++;
  */

  // Substract from max cost because the heap is set up to return the max
  // element.  This could of course be changed.
  heap_insert(plan->heap, PLAN_MAX_COST - cell->plan_cost, cell);

  return;
}


// Pop a plan location from the queue
plan_cell_t *plan_pop(plan_t *plan)
{
  /*
  int i;
  plan_cell_t *cell;
  
  if (plan->queue_len == 0)
    return NULL;

  i = plan->queue_start % plan->queue_size;
  cell = plan->queue[i];
  plan->queue_start++;
  plan->queue_len--;
  */

  if(heap_empty(plan->heap))
    return(NULL);
  else
    return(heap_extract_max(plan->heap));
}
