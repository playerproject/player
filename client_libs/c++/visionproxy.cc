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
 * client-side vision device 
 */

#include <visionproxy.h>
#include <netinet/in.h>
#include <string.h>
#ifdef PLAYER_SOLARIS
  #include <strings.h>
#endif

void VisionProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  static bool firsttime = true;

  if(firsttime)
  {
    bzero(num_blobs,sizeof(num_blobs));
    bzero(blobs,sizeof(blobs));
    firsttime = false;
  }

  if(hdr.size > ACTS_TOTAL_MAX_SIZE)
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected less than %d bytes of vision data, but "
              "received %d. Unexpected results may ensue.\n",
              ACTS_TOTAL_MAX_SIZE,hdr.size);
  }
  // fill the special vision buffer.
  int i,j,k;
  int area;
  int bufptr;

  bufptr = ACTS_HEADER_SIZE;
  for(i=0;i<ACTS_NUM_CHANNELS;i++)
  {
    //printf("%d blobs starting at %d on %d\n", buf[2*i+1]-1,buf[2*i]-1,i+1);
    if(!(buffer[2*i+1]-1))
    {
      num_blobs[i] = 0;
    }
    else
    {
      /* check to see if we need more room */
      if(buffer[2*i+1]-1 > num_blobs[i])
      {
        if(blobs[i])
          delete blobs[i];
        blobs[i] = new Blob[buffer[2*i+1]-1];
      }
      for(j=0;j<buffer[2*i+1]-1;j++)
      {
        /* first compute the area */
        area=0;
        for(k=0;k<4;k++)
        {
          area = area << 6;
          area |= buffer[bufptr + k] - 1;
        }
        blobs[i][j].area = area;
        blobs[i][j].x = buffer[bufptr + 4] - 1;
        blobs[i][j].y = buffer[bufptr + 5] - 1;
        blobs[i][j].left = buffer[bufptr + 6] - 1;
        blobs[i][j].right = buffer[bufptr + 7] - 1;
        blobs[i][j].top = buffer[bufptr + 8] - 1;
        blobs[i][j].bottom = buffer[bufptr + 9] - 1;

        bufptr += ACTS_BLOB_SIZE;
      }
      num_blobs[i] = buffer[2*i+1]-1;
    }
  }
}

// interface that all proxies SHOULD provide
void VisionProxy::Print()
{
  printf("#Vision(%d:%d) - %c\n", device, index, access);
  for(int i=0;i<ACTS_NUM_CHANNELS;i++)
  {
    if(num_blobs[i])
    {
      printf("#Channel %d:\n", i);
      for(int j=0;j<num_blobs[i];j++)
      {
        printf("  blob %d:\n"
               "             area: %d\n"
               "                X: %d\n"
               "                Y: %d\n"
               "             Left: %d\n"
               "            Right: %d\n"
               "              Top: %d\n"
               "           Bottom: %d\n",
               j+1,
               blobs[i][j].area,
               blobs[i][j].x,
               blobs[i][j].y,
               blobs[i][j].left,
               blobs[i][j].right,
               blobs[i][j].top,
               blobs[i][j].bottom);
      }
    }
  }
}

