/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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

/*
 *   the internal map.
 */
#ifndef _WORLDMODEL_H
#define _WORLDMODEL_H

#include "mcl_types.h"

#include <stdint.h>


class WorldModel
{
    public:	// supposed to be private, but for efficiency
	// the size of a world
	uint32_t width, height;
	// the number of pixels per meter
	uint16_t ppm;
	// the size of a grid
	float grid_size;
	// occupancy grid map
	uint8_t *map;
	// the maximum range of a distance sensor
	uint16_t max_range;
	// threshold for empty/occupied decision
	uint8_t threshold;
	// is a map loaded ?
	bool loaded;

    protected:
	// Load the occupancy grid map data from a PGM file
	bool readFromPGM(const char* filename);
	// Load the occupancy grid map data from a compressed PGM file
	bool readFromCompressedPGM(const char* filename);

    public:
	// constructor
	WorldModel(const char* filename=0,	// map file
		   uint16_t ppm=0,		// pixels per meter
		   uint16_t max_range=8000,	// maximum range of a distance sensor
		   uint8_t threshold=240);	// threshold for empty/occupied
	// destructor
	~WorldModel(void);

	// Load the occupancy grid map data from a file
	bool readMap(const char* filename, uint16_t ppm);

	// compute the distance to the closest wall from (x,y,a)
	uint16_t estimateRange(pose_t from);

	// access functions
	bool isLoaded(void) { return loaded; }
	int32_t Width(void) { return int(width * grid_size + 0.5); }
	int32_t Height(void) { return int(height * grid_size + 0.5); }
};


#endif
