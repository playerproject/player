
/**************************************************************************
 * Desc: Local map GUI functions
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <rtk.h>
#include "imap.h"


// Draw the occupancy imap
void imap_draw(imap_t *imap, rtk_fig_t *fig)
{
  int i, j;
  int col;
  imap_cell_t *cell;
  uint16_t *pixel;

  // Draw occupancy
  for (j = 0; j < imap->size_y; j++)
  {
    for (i =  0; i < imap->size_x; i++)
    {
      cell = imap->cells + IMAP_INDEX(imap, i, j);
      pixel = imap->image + (j * imap->size_x + i);

      col = 127 - 127 * cell->occ_state;
      *pixel = RTK_RGB16(col, col, col);

      // TESTING
      //col = 255 - (int) (255 * cell->occ_dist / imap->max_occ_dist);
      //col = 255 - (int) (255 * cell->emp_dist / imap->max_occ_dist);
      //*pixel = RTK_RGB16(0, col, 0);
    }
  }

  // Draw the entire occupancy imap as an image
  rtk_fig_image(fig, imap->origin_x, imap->origin_y, 0,
                imap->scale, imap->size_x, imap->size_y, 16, imap->image, NULL);

  // TESTING
  //imap_draw_dist(imap, fig);
  
  return;
}


// Draw the occupancy offsets
void imap_draw_dist(imap_t *imap, rtk_fig_t *fig)
{
  int i, j;
  int di, dj;
  double dr;
  double ax, ay, bx, by;
  imap_cell_t *cell;

  rtk_fig_color(fig, 1, 0, 0);
  
  for (j = 0; j < imap->size_y; j++)
  {
    for (i =  0; i < imap->size_x; i++)
    {
      cell = imap->cells + IMAP_INDEX(imap, i, j);
      dr = cell->occ_dist;
      di = cell->occ_di;
      dj = cell->occ_dj;

      if (dr >= imap->max_occ_dist)
        continue;

      if (di == 0 && dj == 0)
        continue;

      ax = IMAP_WXGX(imap, i);
      ay = IMAP_WYGY(imap, j);

      bx = IMAP_WXGX(imap, i + di);
      by = IMAP_WYGY(imap, j + dj);

      rtk_fig_arrow_ex(fig, ax, ay, bx, by, 0.02);
    }
  }

  return;
}


