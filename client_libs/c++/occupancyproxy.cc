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
 * $Id$
 *
 * client-side sonar device 
 */

//#define DEBUG

#include <occupancyproxy.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>

#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

#define PIXEL_ALLOCATION 100

  // the client calls this method to make a new proxy
  //   leave access empty to start unconnected
OccupancyProxy::OccupancyProxy(PlayerClient* pc, unsigned short index, 
	   unsigned char access = 'c') :
  ClientProxy(pc,PLAYER_OCCUPANCY_TYPE,index,access)
{
  // init the occupancy grid structure
  pixels = NULL;
  num_pixels = 0;
  alloc_pixels = 0;
  width = height = 0;
  ppm = 0;
};

void OccupancyProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
#ifdef DEBUG
   puts( "OccupancyProxy::FillData()" ); 
#endif

   if( hdr.size == 0 ) return; // nothing to do here

   if( hdr.size < sizeof(player_occupancy_data_t) && hdr.size != 0 )
     {
       if(player_debug_level(-1) >= 1)
	 fprintf(stderr,"WARNING: expected at least %d (or zero) bytes of "
		 "occupancy data, but "
		 "received %d. Unexpected results may ensue.\n",
		 sizeof(player_occupancy_data_t),hdr.size);
     }

   
   printf( "OP recv buffer %d bytes\n", hdr.size );

   // import the data
   player_occupancy_data_t* odata = (player_occupancy_data_t*)buffer;

   num_pixels = odata->num_pixels;
   if( num_pixels == 0 ) return; // do nothing

   // import the grid parameters
   width = odata->width;
   height = odata->height;
   ppm = odata->ppm;

   printf( "OP recv %d pixels [%d,%d] at %d ppm\n", 
	   num_pixels, width, height, ppm );
   
   
   // if there is a pre-update callback, call it.
   if( preUpdateCallback ) (*preUpdateCallback)();
   
   if( pixels ) delete[] pixels;
   
   pixels = new pixel_t[ num_pixels ];
   
   // copy the existing data into the new buffer
   memcpy( pixels, 
	   buffer + sizeof(player_occupancy_data_t), 
	   num_pixels * sizeof( pixel_t ) );

   // if there is a post-update callback then call it
   if( postUpdateCallback ) (*postUpdateCallback)();
}

// interface that all proxies SHOULD provide
void OccupancyProxy::Print()
{
  printf( "Occupancy: \n"
	  "\tWidth\tHeight\tPPM\tpts\tspace\n"
	  "\t%d\t%d\t%d\t%d\t%d\n",
	  width, height, ppm, num_pixels, alloc_pixels );
   
  fflush( stdout );
}











