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
 * client-side blobfinder device 
 */

#include <playerclient.h>
#include <netinet/in.h>

BlobfinderProxy::BlobfinderProxy(PlayerClient* pc, unsigned short index, 
            unsigned char access):
            ClientProxy(pc,PLAYER_BLOBFINDER_CODE,index,access)
{
  // zero everything
  bzero(num_blobs,sizeof(num_blobs));
  bzero(blobs,sizeof(blobs)); 
}

BlobfinderProxy::~BlobfinderProxy()
{
  // delete Blob structures
  for(int i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
  {
    if(blobs[i])
      delete blobs[i];
  }
}

void BlobfinderProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  static bool firsttime = true;

  if(firsttime)
  {
    bzero(num_blobs,sizeof(num_blobs));
    bzero(blobs,sizeof(blobs));
    firsttime = false;
  }

  if(hdr.size > sizeof(player_blobfinder_data_t))
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected less than %d bytes of blobfinder data, but "
              "received %d. Unexpected results may ensue.\n",
              sizeof(player_blobfinder_data_t),hdr.size);
  }
 
  // get the dimensions
  width = ntohs(((player_blobfinder_data_t*)buffer)->width);
  height = ntohs(((player_blobfinder_data_t*)buffer)->height);
 
  // fill the special blobfinder buffer.
  int tmp_numblobs,tmp_index;
  for(int i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
  {
    tmp_numblobs = ntohs(((player_blobfinder_data_t*)buffer)->header[i].num);
    tmp_index = ntohs(((player_blobfinder_data_t*)buffer)->header[i].index);

    if(tmp_numblobs <= 0)
    {
      num_blobs[i] = 0;
    }
    else
    {
      /* check to see if we need more room */
      if(tmp_numblobs > num_blobs[i])
      {
        if(blobs[i])
          delete blobs[i];
        if(!(blobs[i] = new Blob[tmp_numblobs]))
        {
          if(player_debug_level(-1)>=0)
            fputs("BlobfinderProxy::FillData(): new failed.  Out of memory?",
                  stderr);
          return;
        }
      }
      for(int j=0;j<tmp_numblobs;j++)
      {
        blobs[i][j].color = 
                ntohl(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].color);
        blobs[i][j].area = 
                ntohl(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].area);
        blobs[i][j].x = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].x);
        blobs[i][j].y = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].y);
        blobs[i][j].left = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].left);
        blobs[i][j].right = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].right);
        blobs[i][j].top = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].top);
        blobs[i][j].bottom = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].bottom);
        blobs[i][j].range = 
                ntohs(((player_blobfinder_data_t*)buffer)->blobs[tmp_index+j].range);
      }
      num_blobs[i] = tmp_numblobs;
    }
  }
}

// interface that all proxies SHOULD provide
void BlobfinderProxy::Print()
{
  printf("#Blobfinder(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  for(int i=0;i<PLAYER_BLOBFINDER_MAX_CHANNELS;i++)
  {
    if(num_blobs[i])
    {
      printf("#Channel %d (%d blob(s))\n", i,num_blobs[i]);
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

