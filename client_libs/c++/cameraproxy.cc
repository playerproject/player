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
 * client-side camera device 
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

#include "playerpacket.h"

// For some reason, having this before the other includes makes it so 
// that it does not output any data.  Might want to look into that

#if HAVE_CONFIG_H
  #include "config.h"
#endif

CameraProxy::CameraProxy( PlayerClient *pc, unsigned short index,
    unsigned char access)
  : ClientProxy(pc, PLAYER_CAMERA_CODE, index, access)
{
  this->frameNo = 0;

  this->image = (uint8_t*)calloc(1,PLAYER_CAMERA_IMAGE_SIZE);
  assert(this->image);
}

CameraProxy::~CameraProxy()
{
  free(this->image);
}

void CameraProxy::FillData( player_msghdr_t hdr, const char *buffer)
{
  size_t size;

  size = sizeof(player_camera_data_t) -
         sizeof(uint8_t[PLAYER_CAMERA_IMAGE_SIZE]) +
         ntohl(((player_camera_data_t*)buffer)->image_size);

  if (hdr.size != size)
  {
    if (player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of camera data, but "
          "received %d. Unexpected results may ensur.\n",
          sizeof(player_camera_data_t), hdr.size);
  }

  this->width = ntohs( ((player_camera_data_t*)buffer)->width);
  this->height = ntohs( ((player_camera_data_t*)buffer)->height);

  // to keep this short, we need to change the depth datatype, otherwise use the second line
  // this->depth = ntohs( ((player_camera_data_t*)buffer)->depth);
  this->depth = ((player_camera_data_t*)buffer)->bpp;

  this->imageSize = ntohl( ((player_camera_data_t*)buffer)->image_size);
  memcpy(this->image, ((player_camera_data_t*)buffer)->image, this->imageSize);
}

// interface that all proxies SHOULD provide
void
CameraProxy::Print()
{
  printf("#Camera(%d:%d) - %c\n", m_device_id.code,
           m_device_id.index, access);

  printf("Height(%d px), Width(%d px), Depth(%d bit), ImageSize(%d bytes)\n",
         this->width,this->height,this->depth,this->imageSize);
}

void CameraProxy::SaveFrame(const char *prefix)
{
  FILE *file;

  snprintf(this->filename, sizeof(this->filename), "%s-%04d.jpg", prefix, this->frameNo++);

  file = fopen(this->filename, "w+");

  fwrite(this->image,1,this->imageSize, file);

  fclose(file);
}

void CameraProxy::Decompress()
{
  
#if HAVE_JPEGLIB_H
  int dst_size;
  unsigned char *dst;

  if (this->compression != PLAYER_CAMERA_COMPRESS_JPEG)
  {
    perror("CameraProxy::Decompress() not a JPEG image, not good!\n");
    //return;
  }

  if (this->depth != 24)
  {
    perror("CameraProxy::Decompress() currently only supports "
           "24-bit images\n");
    return;
  }

  // Create a temp buffer
  dst_size = this->width * this->height * this->depth / 8;
  dst = static_cast<unsigned char *>(malloc(dst_size));

  // Decompress into temp buffer
  jpeg_decompress(dst, dst_size, this->image, this->imageSize);

  // Copy uncompress image
  this->imageSize = dst_size;
  assert(dst_size < PLAYER_CAMERA_IMAGE_SIZE*sizeof(uint8_t));
  memcpy(this->image, dst, dst_size);

  // Pixels are now raw
  this->compression = PLAYER_CAMERA_COMPRESS_RAW;

#else

  perror("JPEG decompression support was not included at compile-time");

#endif

} 

