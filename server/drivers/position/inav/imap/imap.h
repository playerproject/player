
/**************************************************************************
 * Desc: Incremental map building
 * Author: Andrew Howard
 * Date: 10 Oct 2002
 * CVS: $Id$
 **************************************************************************/

#ifndef IMAP_H
#define IMAP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// For GUI stuff
struct _rtk_fig_t;


// Description for a single map cell.  All values must be invariant
// under translation.
typedef struct _imap_cell_t
{
  // Occupancy value
  int occ_value;
  
  // Occupancy state (-1 = free, 0 = unknown, +1 = occ)
  int occ_state;

  // Distance to the nearest occupied cell
  float occ_dist;

  // Offset to the nearest occupied cell
  int occ_di, occ_dj;

} imap_cell_t;


// Distance LUT element
typedef struct
{
  // Cell index offset
  int di, dj;

  // Cell range
  double dr;
  
} imap_dist_lut_t;


// Description for a imap
typedef struct
{
  // Map origin; the map is a viewport onto a conceptual larger map.
  double origin_x, origin_y;
  
  // Map scale (m/cell)
  double scale;

  // Map dimensions (number of cells)
  int size_x, size_y;

  // Model parameters
  int model_occ_inc, model_emp_inc;
  int model_occ_max, model_emp_min;
  int model_occ_thresh, model_emp_thresh;

  // Maximum distance to occupied cells
  double max_occ_dist;

  // Maximum fitting distance (outlier rejection)
  double max_fit_dist;

  // Distance LUT
  int dist_lut_len;
  imap_dist_lut_t *dist_lut;

  // The map data, stored as a grid
  imap_cell_t *cells;
  
  // Image data (for drawing the map)
  uint16_t *image;

} imap_t;



/**************************************************************************
 * Basic map functions
 **************************************************************************/

// Create a new imap
imap_t *imap_alloc(int size_x, int size_y, double scale,
                   double max_occ_dist, double max_fit_dist);

// Destroy a imap
void imap_free(imap_t *imap);

// Reset the imap
void imap_reset(imap_t *imap);

// Translate the map a discrete number of cells in x and/or y
void imap_translate(imap_t *imap, int di, int dj);

// Get the cell at the given point
imap_cell_t *imap_get_cell(imap_t *imap, double ox, double oy, double oa);


/**************************************************************************
 * Range functions
 **************************************************************************/

// Compute the best fit pose between a range scan and the map
double imap_fit_ranges(imap_t *imap, double *ox, double *oy, double *oa,
                       int range_count, double ranges[][2]);

// Add a range scan to the map
int imap_add_ranges(imap_t *imap, double ox, double oy, double oa,
                    int range_count, double ranges[][2]);

// Return the distance to the nearest occupied cell.
double imap_occ_dist(imap_t *imap, double ox, double oy);

// Get a vector that points to the nearest occupied cell.
double imap_occ_vector(imap_t *imap, double ox, double oy, double *dx, double *dy);


/**************************************************************************
 * GUI/diagnostic functions
 **************************************************************************/

// Draw the occupancy grid
void imap_draw_occ(imap_t *imap, struct _rtk_fig_t *fig);

// Draw the occupancy offsets
void imap_draw_dist(imap_t *imap, struct _rtk_fig_t *fig);

// Save the occupancy grid to an image file
int imap_save_occ(imap_t *imap, const char *filename);


/**************************************************************************
 * Map manipulation macros
 **************************************************************************/

// Convert from imap index to world coords
#define IMAP_WXGX(imap, i) (imap->origin_x + ((i) - imap->size_x / 2) * imap->scale)
#define IMAP_WYGY(imap, j) (imap->origin_y + ((j) - imap->size_y / 2) * imap->scale)

// Convert from world coords to imap coords
#define IMAP_GXWX(imap, x) (floor((x - imap->origin_x) / imap->scale + 0.5) + imap->size_x / 2)
#define IMAP_GYWY(imap, y) (floor((y - imap->origin_y) / imap->scale + 0.5) + imap->size_y / 2)

// Test to see if the given imap coords lie within the absolute imap bounds.
#define IMAP_VALID(imap, i, j) ((i >= 0) && (i < imap->size_x) && (j >= 0) && (j < imap->size_y))

// Compute the cell index for the given imap coords.
#define IMAP_INDEX(imap, i, j) ((i) + (j) * imap->size_x)

#ifdef __cplusplus
}
#endif

#endif
