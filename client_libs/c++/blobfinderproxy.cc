/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * $Id$
 *
 * client-side blobfinder device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h> // for memset(3)

BlobfinderProxy::BlobfinderProxy(PlayerClient* pc, unsigned short index, 
            unsigned char access):
            ClientProxy(pc,PLAYER_BLOBFINDER_CODE,index,access)
{
  // zero everything
  blob_count = 0;
  memset(blobs,0,sizeof(blobs)); 
}

BlobfinderProxy::~BlobfinderProxy()
{
}

void BlobfinderProxy::FillData(player_msghdr_t hdr, const char* buffer)
{
  static bool firsttime = true;

  if(firsttime)
  {
    blob_count = 0;
    memset(blobs,0,sizeof(blobs));
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
  blob_count = ntohs(((player_blobfinder_data_t*)buffer)->blob_count);
 
  // fill the special blobfinder buffer.
  for(int i=0;i<blob_count;i++)
  {
    blobs[i].color = 
      ntohl(((player_blobfinder_data_t*)buffer)->blobs[i].color);
    blobs[i].area = 
      ntohl(((player_blobfinder_data_t*)buffer)->blobs[i].area);
    blobs[i].x = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].x);
    blobs[i].y = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].y);
    blobs[i].left = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].left);
    blobs[i].right = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].right);
    blobs[i].top = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].top);
    blobs[i].bottom = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].bottom);
    blobs[i].range = 
      ntohs(((player_blobfinder_data_t*)buffer)->blobs[i].range) / 1e3;
  }
}

// interface that all proxies SHOULD provide
void BlobfinderProxy::Print()
{
  printf("#Blobfinder(%d:%d) - %c\n", m_device_id.code,
         m_device_id.index, access);
  for(int i=0;i<blob_count;i++)
  {
    printf("  blob %d:\n"
           "               id: %d\n"
           "             area: %d\n"
           "                X: %d\n"
           "                Y: %d\n"
           "             Left: %d\n"
           "            Right: %d\n"
           "              Top: %d\n"
           "           Bottom: %d\n",
           i,
           blobs[i].id,
           blobs[i].area,
           blobs[i].x,
           blobs[i].y,
           blobs[i].left,
           blobs[i].right,
           blobs[i].top,
           blobs[i].bottom);
  }
}


/* Auto-set version of SetTrackingColor().  Without any params, it set's the
** tracking color to the current sensor window.  This is useful for, say,
** holding the object to be tracked in front of the camera and letting the
** imager automatically figure out the appropriate RGB max and min values.  */
int BlobfinderProxy::SetTrackingColor()
{
   return SetTrackingColor(-1, -1, -1, -1, -1, -1);
}


/* Manually set the RGB max and min values for the color to track.
** Values range between 0 and 255.  Setting any of the values to 
** -1 will result in auto-setting of tracking color. */
int BlobfinderProxy::SetTrackingColor(int rmin, int rmax, int gmin,
                                      int gmax, int bmin, int bmax)
{
   if (!client)
      return (-1);

   player_blobfinder_color_config_t config;

   config.subtype = PLAYER_BLOBFINDER_SET_COLOR_REQ;
   config.rmin = htons(rmin);
   config.rmax = htons(rmax);
   config.gmin = htons(gmin);
   config.gmax = htons(gmax);
   config.bmin = htons(bmin);
   config.bmax = htons(bmax);

   return(client->Request(m_device_id,(const char*)&config,
                         sizeof(config)));
}


/* Set the imager contrast.  (0-255)*/
int BlobfinderProxy::SetContrast(int contrast)
{
   return SetImagerParams(contrast, -1, -1, -1);
}


/* Set the imager brightness.  (0-255)*/
int BlobfinderProxy::SetBrightness(int brightness)
{
   return SetImagerParams(-1, brightness, -1, -1);
}


/* Set the colormode:
**               (0=RGB/AutoWhiteBalance Off,  1=RGB/AutoWhiteBalance On,
**                2=YCrCB/AWB Off, 3=YCrCb/AWB On)
*/
int BlobfinderProxy::SetColorMode(int colormode)
{
   return SetImagerParams(-1, -1, -1, colormode);
}


/* Set the imager autogain. (0=off, 1=on) */
int BlobfinderProxy::SetAutoGain(int autogain)
{
   return SetImagerParams(-1, -1, autogain, -1);
}


/* Set the imager configuration for the blobfinder device.  This includes:
**       brightness  (0-255)
**       contrast    (0-255)
**       auto gain   (0=off, 1=on)
**       color mode  (0=RGB/AutoWhiteBalance Off,  1=RGB/AutoWhiteBalance On,
**                2=YCrCB/AWB Off, 3=YCrCb/AWB On)
** Any values set to -1 will be left unchanged. */
int BlobfinderProxy::SetImagerParams(int contrast, int brightness,
                                     int autogain, int colormode)
{
   if (!client)
      return (-1);

   player_blobfinder_imager_config_t config;

   config.subtype = PLAYER_BLOBFINDER_SET_IMAGER_PARAMS_REQ;
   config.brightness = htons(brightness);
   config.contrast = htons(contrast);
   config.colormode = (unsigned char)colormode;
   config.autogain = (unsigned char)autogain;

   return(client->Request(m_device_id,(const char*)&config,
                         sizeof(config)));

}
