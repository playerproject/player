#include <stdlib.h>
#include <assert.h>

#include "plan.h"

double _plan_check_path(plan_t* plan, plan_cell_t* s, plan_cell_t* g);

double
plan_get_carrot(plan_t* plan, double* px, double* py, 
                double lx, double ly, double maxdist, double distweight)
{
  plan_cell_t* cell, *ncell;
  int li, lj;
  double dist, d;
  double cost, bestcost;
  char old_occ_state;
  float old_occ_dist;

  li = PLAN_GXWX(plan, lx);
  lj = PLAN_GYWY(plan, ly);

  cell = plan->cells + PLAN_INDEX(plan,li,lj);

  // Latch and clear the obstacle state for the cell I'm in
  cell = plan->cells + PLAN_INDEX(plan, li, lj);
  old_occ_state = cell->occ_state_dyn;
  old_occ_dist = cell->occ_dist_dyn;
  cell->occ_state_dyn = -1;
  cell->occ_dist_dyn = plan->max_radius;

  // Step back from maxdist, looking for the best carrot
  bestcost = -1.0;
  for(dist = maxdist; dist >= plan->scale; dist -= plan->scale)
  {
    // Find a point the required distance ahead, following the cost gradient
    d=plan->scale;
    for(ncell = cell;
        (ncell->plan_next && (d < dist));
        ncell = ncell->plan_next, d+=plan->scale);

    // Check whether the straight-line path is clear
    if((cost = _plan_check_path(plan, cell, ncell)) < 0.0)
    {
      printf("no path from (%d,%d) to (%d,%d)\n",
             cell->ci, cell->cj, ncell->ci, ncell->cj);
      continue;
    }

    // Weight distance
    cost += distweight * (1.0/(dist*dist));
    if((bestcost < 0.0) || (cost < bestcost))
    {
      bestcost = cost;
      *px = PLAN_WXGX(plan,ncell->ci);
      *py = PLAN_WYGY(plan,ncell->cj);
    }
  }
 
  // Restore the obstacle state for the cell I'm in
  cell = plan->cells + PLAN_INDEX(plan, li, lj);
  cell->occ_state_dyn = old_occ_state;
  cell->occ_dist_dyn = old_occ_dist;

  return(bestcost);
}

double
_plan_check_path(plan_t* plan, plan_cell_t* s, plan_cell_t* g)
{
  // Bresenham raytracing
  int x0,x1,y0,y1;
  int x,y;
  int xstep, ystep;
  char steep;
  int tmp;
  int deltax, deltay, error, deltaerr;
  int obscost=0;

  x0 = s->ci;
  y0 = s->cj;
  
  x1 = g->ci;
  y1 = g->cj;

  if(abs(y1-y0) > abs(x1-x0))
    steep = 1;
  else
    steep = 0;

  if(steep)
  {
    tmp = x0;
    x0 = y0;
    y0 = tmp;

    tmp = x1;
    x1 = y1;
    y1 = tmp;
  }

  deltax = abs(x1-x0);
  deltay = abs(y1-y0);
  error = 0;
  deltaerr = deltay;

  x = x0;
  y = y0;

  if(x0 < x1)
    xstep = 1;
  else
    xstep = -1;
  if(y0 < y1)
    ystep = 1;
  else
    ystep = -1;

  if(steep)
  {
    if(plan->cells[PLAN_INDEX(plan,y,x)].occ_dist_dyn < plan->abs_min_radius)
      return -1;
    else if(plan->cells[PLAN_INDEX(plan,y,x)].occ_dist_dyn < plan->max_radius)
      obscost += plan->dist_penalty * 
              (plan->max_radius - 
               plan->cells[PLAN_INDEX(plan,y,x)].occ_dist_dyn);
  }
  else
  {
    if(plan->cells[PLAN_INDEX(plan,x,y)].occ_dist_dyn < plan->abs_min_radius)
      return -1;
    else if(plan->cells[PLAN_INDEX(plan,x,y)].occ_dist_dyn < plan->max_radius)
      obscost += plan->dist_penalty * 
              (plan->max_radius - 
               plan->cells[PLAN_INDEX(plan,x,y)].occ_dist_dyn);
  }

  while(x != (x1 + xstep * 1))
  {
    x += xstep;
    error += deltaerr;
    if(2*error >= deltax)
    {
      y += ystep;
      error -= deltax;
    }

    if(steep)
    {
      if(plan->cells[PLAN_INDEX(plan,y,x)].occ_dist_dyn < plan->abs_min_radius)
        return -1;
      else if(plan->cells[PLAN_INDEX(plan,y,x)].occ_dist_dyn < plan->max_radius)
        obscost += plan->dist_penalty * 
                (plan->max_radius - 
                 plan->cells[PLAN_INDEX(plan,y,x)].occ_dist_dyn);
    }
    else
    {
      if(plan->cells[PLAN_INDEX(plan,x,y)].occ_dist_dyn < plan->abs_min_radius)
        return -1;
      else if(plan->cells[PLAN_INDEX(plan,x,y)].occ_dist_dyn < plan->max_radius)
        obscost += plan->dist_penalty * 
                (plan->max_radius - 
                 plan->cells[PLAN_INDEX(plan,x,y)].occ_dist_dyn);
    }
  }

  return(obscost);
}


