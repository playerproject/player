#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "plan.h"

#define USAGE "USAGE: test <map.png> <res> <lx> <ly> <gx> <gy>"

// compute linear index for given map coords
#define MAP_IDX(sx, i, j) ((sx) * (j) + (i))

double get_time(void);

int read_map_from_image(int* size_x, int* size_y, char** mapdata, 
       			const char* fname, int negate);

int
main(int argc, char** argv)
{
  char* fname;
  double res;
  double lx,ly,gx,gy;


  char* mapdata;
  int sx, sy;
  int i, j;

  plan_t* plan;
  double robot_radius=0.25;
  double safety_dist=0.2;
  double max_radius=1.0;
  double dist_penalty=1.0;;

  double t0, t1, t2, t3;

  if(argc < 7)
  {
    puts(USAGE);
    exit(-1);
  }

  fname = argv[1];
  res = atof(argv[2]);
  lx = atof(argv[3]);
  ly = atof(argv[4]);
  gx = atof(argv[5]);
  gy = atof(argv[6]);

  assert(read_map_from_image(&sx, &sy, &mapdata, fname, 0) == 0);

  assert((plan = plan_alloc(robot_radius+safety_dist,
			    robot_radius+safety_dist,
			    max_radius,
			    dist_penalty)));

  plan->scale = res;
  plan->size_x = sx;
  plan->size_y = sy;
  plan->origin_x = 0.0;
  plan->origin_y = 0.0;

  // Set the bounds to search the whole grid.
  plan_set_bounds(plan, 0, 0, plan->size_x - 1, plan->size_y - 1);
  
  // allocate space for map cells
  assert(plan->cells == NULL);
  assert((plan->cells = 
	  (plan_cell_t*)realloc(plan->cells,
				(sx * sy * sizeof(plan_cell_t)))));
  plan_reset(plan);

  for(j=0;j<sy;j++)
  {
    for(i=0;i<sx;i++)
    {
      plan->cells[PLAN_INDEX(plan,i,j)].occ_dist = max_radius;
      if((plan->cells[PLAN_INDEX(plan,i,j)].occ_state = 
          mapdata[MAP_IDX(sx,i,j)]) >= 0)
        plan->cells[PLAN_INDEX(plan,i,j)].occ_dist = 0;
    }
  }

  free(mapdata);

  t0 = get_time();
  plan_update_cspace(plan,NULL);
  t1 = get_time();

  // compute costs to the new goal
  plan_update_plan(plan, gx, gy);
  t2 = get_time();

  // compute a path to the goal from the current position
  plan_update_waypoints(plan, lx, ly);
  t3 = get_time();

  printf("cspace: %.6lf\n", t1-t0);
  printf("update: %.6lf\n", t2-t1);
  printf("waypnt: %.6lf\n", t3-t2);

  if(plan->waypoint_count == 0)
  {
    puts("no path");
  }
  else
  {
    for(i=0;i<plan->waypoint_count;i++)
    {
      double wx, wy;
      plan_convert_waypoint(plan, plan->waypoints[i], &wx, &wy);
      printf("%d: (%d,%d) : (%.3lf,%.3lf)\n",
             i, plan->waypoints[i]->ci, plan->waypoints[i]->cj, wx, wy);
    }
  }

  plan_free(plan);

  return(0);
}

int
read_map_from_image(int* size_x, int* size_y, char** mapdata, 
                    const char* fname, int negate)
{
  GdkPixbuf* pixbuf;
  guchar* pixels;
  guchar* p;
  int rowstride, n_channels, bps;
  GError* error = NULL;
  int i,j,k;
  double occ;
  int color_sum;
  double color_avg;

  // Initialize glib
  g_type_init();

  printf("MapFile loading image file: %s...", fname);
  fflush(stdout);

  // Read the image
  if(!(pixbuf = gdk_pixbuf_new_from_file(fname, &error)))
  {
    printf("failed to open image file %s", fname);
    return(-1);
  }

  *size_x = gdk_pixbuf_get_width(pixbuf);
  *size_y = gdk_pixbuf_get_height(pixbuf);

  assert(*mapdata = (char*)malloc(sizeof(char) * (*size_x) * (*size_y)));

  rowstride = gdk_pixbuf_get_rowstride(pixbuf);
  bps = gdk_pixbuf_get_bits_per_sample(pixbuf)/8;
  n_channels = gdk_pixbuf_get_n_channels(pixbuf);
  if(gdk_pixbuf_get_has_alpha(pixbuf))
    n_channels++;

  // Read data
  pixels = gdk_pixbuf_get_pixels(pixbuf);
  for(j = 0; j < *size_y; j++)
  {
    for (i = 0; i < *size_x; i++)
    {
      p = pixels + j*rowstride + i*n_channels*bps;
      color_sum = 0;
      for(k=0;k<n_channels;k++)
        color_sum += *(p + (k * bps));
      color_avg = color_sum / (double)n_channels;

      if(negate)
        occ = color_avg / 255.0;
      else
        occ = (255 - color_avg) / 255.0;
      if(occ > 0.95)
        (*mapdata)[MAP_IDX(*size_x,i,*size_y - j - 1)] = +1;
      else if(occ < 0.1)
        (*mapdata)[MAP_IDX(*size_x,i,*size_y - j - 1)] = -1;
      else
        (*mapdata)[MAP_IDX(*size_x,i,*size_y - j - 1)] = 0;
    }
  }

  gdk_pixbuf_unref(pixbuf);

  puts("Done.");
  printf("MapFile read a %d X %d map\n", *size_x, *size_y);
  return(0);
}

double 
get_time(void)
{
  struct timeval curr;
  gettimeofday(&curr,NULL);
  return(curr.tv_sec + curr.tv_usec / 1e6);
}
