
/**************************************************************************
 * Desc: Path planning
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
**************************************************************************/

#ifndef PLAN_H
#define PLAN_H

#ifdef __cplusplus
extern "C" {
#endif

// Forward declarations
struct _rtk_fig_t *fig;


// Description for a grid single cell
typedef struct _plan_cell_t
{
  // Cell index in grid map
  int ci, cj;
  
  // Occupancy state (-1 = free, 0 = unknown, +1 = occ)
  int occ_state;

  // Distance to the nearest occupied cell
  float occ_dist;

  // Distance (cost) to the goal
  float plan_cost;

  // The next cell in the plan
  struct _plan_cell_t *plan_next;
  
} plan_cell_t;


// Planner info
typedef struct
{
  // Grid dimensions (number of cells)
  int size_x, size_y;

  // Grid scale (m/cell)
  double scale;

  // Effective robot radius
  double des_min_radius, abs_min_radius;

  // Max radius we will consider
  double max_radius;

  // Penalty factor for cells inside the max radius
  double dist_penalty;

  // The grid data
  plan_cell_t *cells;
  
  // Queue of cells to update
  int queue_start, queue_len, queue_size;
  plan_cell_t **queue;

  // Waypoints
  int waypoint_count, waypoint_size;
  plan_cell_t **waypoints;

  // Image data (for drawing)
  //uint16_t *image;
  
} plan_t;


// Create a planner
plan_t *plan_alloc(double abs_min_radius, double des_min_radius,
                   double max_radius, double dist_penalty);

// Destroy a planner
void plan_free(plan_t *plan);

// Reset the plan
void plan_reset(plan_t *plan);

#if 0
// Load the occupancy values from an image file
int plan_load_occ(plan_t *plan, const char *filename, double scale);
#endif

// Construct the configuration space from the occupancy grid.
void plan_update_cspace(plan_t *plan, const char* cachefile);

// Generate the plan
void plan_update_plan(plan_t *plan, double gx, double gy);

// Generate a path to the goal
void plan_update_waypoints(plan_t *plan, double px, double py);

// Get the ith waypoint; returns zero if there are no more waypoints
int plan_get_waypoint(plan_t *plan, int i, double *px, double *py);

// Convert given waypoint cell to global x,y
void plan_convert_waypoint(plan_t* plan, plan_cell_t *waypoint, 
                           double *px, double *py);

// Draw the grid
void plan_draw_grid(plan_t *plan, struct _rtk_fig_t *fig);

// Draw waypoints
void plan_draw_waypoints(plan_t *plan, struct _rtk_fig_t *fig);

// Write the cspace occupancy distance values to a file, one per line.
// Read them back in with plan_read_cspace().
// Returns non-zero on error.
int plan_write_cspace(plan_t *plan, const char* fname, short hash);

// Read the cspace occupancy distance values from a file, one per line.
// Write them in first with plan_read_cspace().
// Returns non-zero on error.
int plan_read_cspace(plan_t *plan, const char* fname, short hash);

// Compute and return the 16-bit MD5 hash of the map data in the given plan
// object.
short plan_md5(plan_t* plan);

/**************************************************************************
 * Plan manipulation macros
 **************************************************************************/

// Convert from origin-at-lower-left-corner (like Stage) to
// origin-at-center.
#define PLAN_SXCX(plan, x) ((x) - (plan->scale * plan->size_x / 2.0))
#define PLAN_SYCY(plan, y) ((y) - (plan->scale * plan->size_y / 2.0))

// Convert from origin-at-center to origin-at-lower-left-corner (like Stage).
#define PLAN_CXSX(plan, x) ((x) + (plan->scale * plan->size_x / 2.0))
#define PLAN_CYSY(plan, y) ((y) + (plan->scale * plan->size_y / 2.0))

// Convert from plan index to world coords
#define PLAN_WXGX(plan, i) (((i) - plan->size_x / 2) * plan->scale)
#define PLAN_WYGY(plan, j) (((j) - plan->size_y / 2) * plan->scale)

// Convert from world coords to plan coords
#define PLAN_GXWX(plan, x) (floor((x) / plan->scale + 0.5) + plan->size_x / 2)
#define PLAN_GYWY(plan, y) (floor((y) / plan->scale + 0.5) + plan->size_y / 2)

// Test to see if the given plan coords lie within the absolute plan bounds.
#define PLAN_VALID(plan, i, j) ((i >= 0) && (i < plan->size_x) && (j >= 0) && (j < plan->size_y))

// Compute the cell index for the given plan coords.
#define PLAN_INDEX(plan, i, j) ((i) + (j) * plan->size_x)

#ifdef __cplusplus
}
#endif

#endif
