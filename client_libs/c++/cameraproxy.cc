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

CameraProxy::CameraProxy( PlayerClient *pc, unsigned short index,
    unsigned char access)
  : ClientProxy(pc, PLAYER_CAMERA_CODE, index, access)
{
  printf("MAX_SIZE? %d\n",PLAYER_MAX_MESSAGE_SIZE);
}

CameraProxy::~CameraProxy()
{
}

void CameraProxy::FillData( player_msghdr_t hdr, const char *buffer)
{
  int32_t size;
  size = sizeof(player_camera_data_t) -
         sizeof(uint8_t[PLAYER_CAMERA_IMAGE_SIZE]) +
         ntohl(((player_camera_data_t*)buffer)->image_size);
  if (hdr.size != size) //sizeof(player_camera_data_t))
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
  this->depth = ((player_camera_data_t*)buffer)->depth;

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


