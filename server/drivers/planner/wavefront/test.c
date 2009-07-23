/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Andrew Howard
 *     Brian Gerkey    
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

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/time.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "plan.h"

#define USAGE "USAGE: test <map.png> <res> <lx> <ly> <gx> <gy>"

// compute linear index for given map coords
#define MAP_IDX(sx, i, j) ((sx) * (j) + (i))

void draw_cspace(plan_t* plan, const char* fname);
void draw_path(plan_t* plan, double lx, double ly, const char* fname);

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
  double robot_radius=0.16;
  double safety_dist=0.05;
  double max_radius=0.25;
  double dist_penalty=1.0;;
  double plan_halfwidth = 5.0;

  double t_c0, t_c1, t_p0, t_p1, t_w0, t_w1;

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
			    dist_penalty,0.5)));

  // allocate space for map cells
  assert(plan->cells == NULL);
  assert((plan->cells = 
	  (plan_cell_t*)realloc(plan->cells,
				(sx * sy * sizeof(plan_cell_t)))));
  
  // Copy over obstacle information from the image data that we read
  for(j=0;j<sy;j++)
  {
    for(i=0;i<sx;i++)
    {
      plan->cells[i+j*sx].occ_state = mapdata[MAP_IDX(sx,i,j)];
    }
  }
  free(mapdata);

  plan->scale = res;
  plan->size_x = sx;
  plan->size_y = sy;
  plan->origin_x = 0.0;
  plan->origin_y = 0.0;
  
  // Do initialization
  plan_init(plan);

  t_c0 = get_time();
  plan_compute_cspace(plan);
  t_c1 = get_time();

  draw_cspace(plan,"cspace.png");

  printf("cspace: %.6lf\n", t_c1-t_c0);
  
  // compute costs to the new goal
  t_p0 = get_time();
  if(plan_do_global(plan, lx, ly, gx, gy) < 0)
    puts("no global path");
  t_p1 = get_time();

  plan_update_waypoints(plan, lx, ly);

  printf("gplan : %.6lf\n", t_p1-t_p0);
  for(i=0;i<plan->waypoint_count;i++)
  {
    double wx, wy;
    plan_convert_waypoint(plan, plan->waypoints[i], &wx, &wy);
    printf("%d: (%d,%d) : (%.3lf,%.3lf)\n",
           i, plan->waypoints[i]->ci, plan->waypoints[i]->cj, wx, wy);
  }

  for(i=0;i<1;i++)
  {
    // compute costs to the new goal
    t_p0 = get_time();
    if(plan_do_local(plan, lx, ly, plan_halfwidth) < 0)
       puts("no local path");
    t_p1 = get_time();

    printf("lplan : %.6lf\n", t_p1-t_p0);

    // compute a path to the goal from the current position
    t_w0 = get_time();
    plan_update_waypoints(plan, lx, ly);
    t_w1 = get_time();

    draw_path(plan,lx,ly,"plan.png");

    printf("waypnt: %.6lf\n", t_w1-t_w0);
    puts("");
  }

  if(plan->waypoint_count == 0)
  {
    puts("no waypoints");
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
  //if(gdk_pixbuf_get_has_alpha(pixbuf))
    //n_channels++;

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
