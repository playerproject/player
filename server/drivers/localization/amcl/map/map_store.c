
/**************************************************************************
 * Desc: Global map storage functions
 * Author: Andrew Howard
 * Date: 6 Feb 2003
 * CVS: $Id$
**************************************************************************/

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "map.h"

// Load a map file (occupancy grid)
int map_load_occ(map_t *map, const char *filename)
{
  FILE *file;
  char magic[3];
  int i, j;
  int ch, occ;
  int width, height;
  map_cell_t *cell;

  // Open file
  file = fopen(filename, "r");
  if (file == NULL)
  {
    PLAYER_ERR2("%s: %s\n", strerror(errno), filename);
    return -1;
  }

  // Read ppm header
  fscanf(file, "%10s \n", magic);
  if (strcmp(magic, "P6") != 0)
  {
    PLAYER_ERR("incorrect image format; must be PPM/binary");
    return -1;
  }

  // Ignore comments
  while ((ch = fgetc(file)) == '#')
    while (fgetc(file) != '\n');
  ungetc(ch, file);

  // Read image dimensions
  fscanf(file, "%d %d \n %*d \n", &width, &height);

  // Allocate space in the map
  map->size_x = width;
  map->size_y = height;
  if (map->cells)
    free(map->cells);
  map->cells = calloc(width * height, sizeof(map->cells[0]));

  // Read in the image
  for (j = height - 1; j >= 0; j--)
  {
    for (i = 0; i < width; i++)
    {
      ch = fgetc(file);
      ch = fgetc(file);
      ch = fgetc(file);

      if (ch < 64)
        occ = +1;
      else if (ch > 192)
        occ = -1;
      else
        occ = 0;

      cell = map->cells + MAP_INDEX(map, i, j);
      cell->occ_state = occ;
    }
  }
  
  fclose(file);
  
  return 0;


  
}



