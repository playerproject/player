
/**************************************************************************
 * Desc: Path planning
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include <playercommon.h>
#include <error.h>

// Use gdk-pixbuf for loading the images
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "plan.h"


// Create a planner
plan_t *plan_alloc(double abs_min_radius, double des_min_radius,
                   double max_radius, double dist_penalty)
{
  plan_t *plan;

  plan = calloc(1, sizeof(plan_t));

  plan->size_x = 0;
  plan->size_y = 0;
  plan->scale = 0.0;

  plan->abs_min_radius = abs_min_radius;
  plan->des_min_radius = des_min_radius;

  plan->max_radius = max_radius;
  plan->dist_penalty = dist_penalty;
  
  plan->queue_start = 0;
  plan->queue_len = 0;
  plan->queue_size = 400000; // HACK: FIX
  plan->queue = calloc(plan->queue_size, sizeof(plan->queue[0]));

  plan->waypoint_count = 0;
  plan->waypoint_size = 100;
  plan->waypoints = calloc(plan->waypoint_size, sizeof(plan->waypoints[0]));
  
  return plan;
}


// Destroy a planner
void plan_free(plan_t *plan)
{
  if (plan->cells)
    free(plan->cells);
  free(plan->queue);
  free(plan);

  return;
}


// Reset the plan
void plan_reset(plan_t *plan)
{
  int i, j;
  plan_cell_t *cell;

  for (j = 0; j < plan->size_y; j++)
  {
    for (i = 0; i < plan->size_x; i++)
    {
      cell = plan->cells + PLAN_INDEX(plan, i, j);
      cell->ci = i;
      cell->cj = j;
      cell->occ_state = 0;
      cell->occ_dist = plan->max_radius;
      cell->plan_cost = 1e12;
      cell->plan_next = NULL;
    }
  }
  plan->waypoint_count = 0;
  return;
}


// Load the occupancy map
int plan_load_occ(plan_t *plan, const char *filename, double scale)
{
  int i, j;
  int occ;
  plan_cell_t *cell;
  GdkPixbuf* pixbuf;
  guchar* pixels;
  guchar* p;
  int rowstride, n_channels, bps;
  GError* error = NULL;

  // Initialize glib
  g_type_init();

  // Read the image
  if(!(pixbuf = gdk_pixbuf_new_from_file(filename, &error)))
  {
    PLAYER_ERROR1("failed to open image file %s", filename);
    return(-1);
  }

  plan->scale = scale;
  plan->size_x = gdk_pixbuf_get_width(pixbuf);
  plan->size_y = gdk_pixbuf_get_height(pixbuf);

  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  bps = gdk_pixbuf_get_bits_per_sample(pixbuf)/8;
  n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  if(gdk_pixbuf_get_has_alpha(pixbuf))
    n_channels++;

  // Allocate space
  if (plan->cells)
    free(plan->cells);
  plan->cells = calloc(plan->size_x * plan->size_y, sizeof(plan_cell_t));
  //plan->image = calloc(plan->size_x * plan->size_y, sizeof(uint16_t));

  // Reset the grid
  plan_reset(plan);
    
  // Read data
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  for(j = 0; j < plan->size_y; j++)
  {
    for (i = 0; i < plan->size_x; i++)
    {
      if (!PLAN_VALID(plan, i, plan->size_y-j))
        continue;

      p = pixels + j*rowstride + i*n_channels*bps;
      
      cell = plan->cells + PLAN_INDEX(plan, i, plan->size_y - j);

      // HACK
      occ = 100 - 100 * (*(p+bps) / 255);
      if (occ > 90)
      {
        cell->occ_state = +1;
        cell->occ_dist = 0;
      }
      else if (occ < 10)
        cell->occ_state = -1;
      else
      {
        cell->occ_state = 0;
        cell->occ_dist = 0;
      }
    }
  }
  
  gdk_pixbuf_unref(pixbuf);
  
  return 0;
}


// Construct the configuration space from the occupancy grid.
// This treats both occupied and unknown cells as bad.
void plan_update_cspace(plan_t *plan)
{
  int i, j;
  int di, dj, dn;
  double r;
  plan_cell_t *cell, *ncell;

  dn = (int) ceil(plan->max_radius / plan->scale);
  
  for (j = 0; j < plan->size_y; j++)
  {
    for (i = 0; i < plan->size_x; i++)
    {
      cell = plan->cells + PLAN_INDEX(plan, i, j);
      if (cell->occ_state < 0)
        continue;

      cell->occ_dist = plan->max_radius;
      //cell->occ_dist = 0.0;

      for (dj = -dn; dj <= +dn; dj++)
      {
        for (di = -dn; di <= +dn; di++)
        {
          if (!PLAN_VALID(plan, i + di, j + dj))            
            continue;
          ncell = plan->cells + PLAN_INDEX(plan, i + di, j + dj);
          
          r = plan->scale * sqrt(di * di + dj * dj);
          if (r < ncell->occ_dist)
            ncell->occ_dist = r;
        }
      }
    }
  }
  return;
}

// Write the cspace occupancy distance values to a file, one per line.
// Read them back in with plan_read_cspace().
// Returns non-zero on error.
int plan_write_cspace(plan_t *plan, const char* fname)
{
  plan_cell_t* cell;
  int i,j;
  FILE* fp;

  if(!(fp = fopen(fname,"w+")))
  {
    perror("failed to open file");
    return(-1);
  }

  for(j = 0; j < plan->size_y; j++)
  {
    for(i = 0; i < plan->size_x; i++)
    {
      cell = plan->cells + PLAN_INDEX(plan, i, j);
      fprintf(fp,"%f\n", cell->occ_dist);
    }
  }

  if(fclose(fp))
  {
    perror("failed to close file");
    return(-1);
  }

  return(0);
}

// Read the cspace occupancy distance values from a file, one per line.
// Write them in first with plan_read_cspace().
// Returns non-zero on error.
int plan_read_cspace(plan_t *plan, const char* fname)
{
  plan_cell_t* cell;
  int i,j;
  FILE* fp;

  if(!(fp = fopen(fname,"r")))
  {
    perror("failed to open file");
    return(-1);
  }

  for(j = 0; j < plan->size_y; j++)
  {
    for(i = 0; i < plan->size_x; i++)
    {
      cell = plan->cells + PLAN_INDEX(plan, i, j);
      fscanf(fp,"%f", &(cell->occ_dist));
    }
  }

  if(fclose(fp))
  {
    perror("failed to close file");
    return(-1);
  }

  return(0);
}
