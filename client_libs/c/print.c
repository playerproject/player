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
 * functions to print out data values in the C client
 */
#include "playercclient.h"
#include <stdio.h>

/*
 * this function needs to be updated to read the new vision packet format
 *    - BPG
 */
/*
void player_print_vision(player_vision_data_t data)
{
  int i,j,k;
  int area;
  int bufptr;
  char* buf = (char*)&data;

  bufptr = ACTS_HEADER_SIZE;
  for(i=0;i<ACTS_NUM_CHANNELS;i++)
  {
    if(!(buf[2*i+1]-1))
    {
      // no blobs on this channel 
      //printf("no blobs on channel %d\n", i);
    }
    else
    {
      printf("%d blob(s) on channel %d\n", buf[2*i+1]-1, i);
      for(j=0;j<buf[2*i+1]-1;j++)
      {
        // first compute the area 
        area=0;
        for(k=0;k<4;k++)
        {
          area = area << 6;
          area |= buf[bufptr + k] - 1;
        }
        printf("  area:%d\n", area);
        printf("  x:%d\n", (unsigned char)(buf[bufptr + 4] - 1));
        printf("  y:%d\n", (unsigned char)(buf[bufptr + 5] - 1));
        printf("  left:%d\n", (unsigned char)(buf[bufptr + 6] - 1));
        printf("  right:%d\n", (unsigned char)(buf[bufptr + 7] - 1));
        printf("  top:%d\n", (unsigned char)(buf[bufptr + 8] - 1));
        printf("  bottom:%d\n", (unsigned char)(buf[bufptr + 9] - 1));

        bufptr += ACTS_BLOB_SIZE;
      }
    }
  }
}
*/

void player_print_ptz(player_ptz_data_t data)
{
  printf("pan:%d\ttilt:%d\tzoom:%d\n", data.pan,data.tilt,data.zoom);
}

void player_print_laser(player_laser_data_t data)
{
  int i;
  for(i=0;i<PLAYER_LASER_MAX_SAMPLES;i++)
    printf("laser(%d) = %d\n", i, data.ranges[i]);
}
void player_print_sonar(player_sonar_data_t data)
{
  int i;
  for(i=0;i<PLAYER_SONAR_MAX_SAMPLES;i++)
    printf("sonar(%d): %d\n", i, data.ranges[i]);
}
void player_print_position(player_position_data_t data)
{
  printf("pos: (%d,%d,%d)\n", data.xpos,data.ypos,data.yaw);
  printf("speed: %d  turnrate: %d\n", data.xspeed, data.yawspeed);
  //printf("compass: %d\n", data.compass);
  printf("stalls: %d\n", data.stall);
}



