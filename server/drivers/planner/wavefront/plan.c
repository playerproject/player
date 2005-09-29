
/**************************************************************************
 * Desc: Path planning
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#if HAVE_CONFIG_H
  #include <config.h>
#endif
#if HAVE_OPENSSL_MD5_H && HAVE_LIBCRYPTO
  #include <openssl/md5.h>
#endif

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>



#include <playercommon.h>
#include <error.h>

#include "plan.h"

#if HAVE_OPENSSL_MD5_H && HAVE_LIBCRYPTO
// length of the hash, in unsigned ints
#define HASH_LEN (MD5_DIGEST_LENGTH / sizeof(unsigned int))
#endif

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
  free(plan->waypoints);
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

// Construct the configuration space from the occupancy grid.
// This treats both occupied and unknown cells as bad.
// 
// If cachefile is non-NULL, then we try to read the c-space from that
// file.  If that fails, then we construct the c-space as per normal and
// then write it out to cachefile.
void 
plan_update_cspace(plan_t *plan, const char* cachefile)
{
  int i, j;
  int di, dj, dn;
  double r;
  plan_cell_t *cell, *ncell;

#if HAVE_OPENSSL_MD5_H && HAVE_LIBCRYPTO
  unsigned int hash[HASH_LEN];
  plan_md5(hash, plan);
  if(cachefile)
  {
    PLAYER_MSG1(1,"Trying to read c-space from file %s", cachefile);
    if(plan_read_cspace(plan,cachefile,hash) == 0)
    {
      // Reading from the cache file worked; we're done here.
      PLAYER_MSG1(1,"Successfully read c-space from file %s", cachefile);
      return;
    }
    PLAYER_MSG1(1, "Failed to read c-space from file %s", cachefile);
  }
#endif

  PLAYER_MSG0(1,"Generating C-space....");
          
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

#if HAVE_OPENSSL_MD5_H && HAVE_LIBCRYPTO
  if(cachefile)
    plan_write_cspace(plan,cachefile, (unsigned int*)hash);
#endif

  PLAYER_MSG0(1,"Done.");
}

// Write the cspace occupancy distance values to a file, one per line.
// Read them back in with plan_read_cspace().
// Returns non-zero on error.
int 
plan_write_cspace(plan_t *plan, const char* fname, unsigned int* hash)
{
  plan_cell_t* cell;
  int i,j;
  FILE* fp;

  if(!(fp = fopen(fname,"w+")))
  {
    PLAYER_MSG2(2,"Failed to open file %s to write c-space: %s",
                fname,strerror(errno));
    return(-1);
  }

  fprintf(fp,"%d\n%d\n", plan->size_x, plan->size_y);
  fprintf(fp,"%.3lf\n%.3lf\n", plan->origin_x, plan->origin_y);
  fprintf(fp,"%.3lf\n%.3lf\n", plan->scale,plan->max_radius);
  for(i=0;i<HASH_LEN;i++)
    fprintf(fp,"%08X", hash[i]);
  fprintf(fp,"\n");

  for(j = 0; j < plan->size_y; j++)
  {
    for(i = 0; i < plan->size_x; i++)
    {
      cell = plan->cells + PLAN_INDEX(plan, i, j);
      fprintf(fp,"%.3f\n", cell->occ_dist);
    }
  }

  fclose(fp);
  return(0);
}

// Read the cspace occupancy distance values from a file, one per line.
// Write them in first with plan_read_cspace().
// Returns non-zero on error.
int 
plan_read_cspace(plan_t *plan, const char* fname, unsigned int* hash)
{
  plan_cell_t* cell;
  int i,j;
  FILE* fp;
  int size_x, size_y;
  double origin_x, origin_y;
  double scale, max_radius;
  unsigned int cached_hash[HASH_LEN];

  if(!(fp = fopen(fname,"r")))
  {
    PLAYER_MSG1(2,"Failed to open file %s", fname);
    return(-1);
  }
  
  /* Read out the metadata */
  if((fscanf(fp,"%d", &size_x) < 1) ||
     (fscanf(fp,"%d", &size_y) < 1) ||
     (fscanf(fp,"%lf", &origin_x) < 1) ||
     (fscanf(fp,"%lf", &origin_y) < 1) ||
     (fscanf(fp,"%lf", &scale) < 1) ||
     (fscanf(fp,"%lf", &max_radius) < 1))
  {
    PLAYER_MSG1(2,"Failed to read c-space metadata from file %s", fname);
    fclose(fp);
    return(-1);
  }

  for(i=0;i<HASH_LEN;i++)
  {
    if(fscanf(fp,"%08X", cached_hash+i) < 1)
    {
      PLAYER_MSG1(2,"Failed to read c-space metadata from file %s", fname);
      fclose(fp);
      return(-1);
    }
  }

  /* Verify that metadata matches */
  if((size_x != plan->size_x) ||
     (size_y != plan->size_y) ||
     (fabs(origin_x - plan->origin_x) > 1e-3) ||
     (fabs(origin_y - plan->origin_y) > 1e-3) ||
     (fabs(scale - plan->scale) > 1e-3) ||
     (fabs(max_radius - plan->max_radius) > 1e-3) ||
     memcmp(cached_hash, hash, sizeof(unsigned int) * HASH_LEN))
  {
    PLAYER_MSG1(2,"Mismatch in c-space metadata read from file %s", fname);
    fclose(fp);
    return(-1);
  }

  for(j = 0; j < plan->size_y; j++)
  {
    for(i = 0; i < plan->size_x; i++)
    {
      cell = plan->cells + PLAN_INDEX(plan, i, j);
      if(fscanf(fp,"%f", &(cell->occ_dist)) < 1)
      {
        PLAYER_MSG3(2,"Failed to read c-space data for cell (%d,%d) from file %s",
                     i,j,fname);
        fclose(fp);
        return(-1);
      }
    }
  }

  fclose(fp);
  return(0);
}



// Compute the 16-byte MD5 hash of the map data in the given plan
// object.
void
plan_md5(unsigned int* digest, plan_t* plan)
{
#if HAVE_OPENSSL_MD5_H && HAVE_LIBCRYPTO
  MD5_CTX c;

  MD5_Init(&c);

  MD5_Update(&c,(const unsigned char*)plan->cells,
             (plan->size_x*plan->size_y)*sizeof(plan_cell_t));

  MD5_Final((unsigned char*)digest,&c);

#else
  PLAYER_ERROR("tried to compute md5 hash, but it's not available");
#endif
}

