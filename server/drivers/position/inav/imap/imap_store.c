
/**************************************************************************
 * Desc: Local map storeage functions
 * Author: Andrew Howard
 * Date: 28 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "imap.h"


// Save the occupancy grid to an image file
int imap_save_occ(imap_t *imap, const char *filename)
{
  int i, j;
  int col;
  imap_cell_t *cell;
  FILE *file;

  file = fopen(filename, "w+");
  if (!file)
  {
    fprintf(stderr, "imap: %s", strerror(errno));
    return -1;
  }

  fprintf(file, "P5\n");
  fprintf(file, "%d %d\n", imap->size_x, imap->size_y);
  fprintf(file, "255\n");
  
  for (j = 0; j < imap->size_y; j++)
  {
    for (i =  0; i < imap->size_x; i++)
    {
      cell = imap->cells + IMAP_INDEX(imap, i, j);
      col = 127 - 127 * cell->occ_state;
      fwrite(&col, 1, 1, file);
    }
  }

  fclose(file);

  return 0;
}


