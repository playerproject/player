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
}

CameraProxy::~CameraProxy()
{
}

void CameraProxy::FillData( player_msghdr_t hdr, const char *buffer)
{
  if (hdr.size != sizeof(player_camera_data_t))
  {
    if (player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: expected %d bytes of camera data, but "
          "received %d. Unexpected results may ensur.\n",
          sizeof(player_camera_data_t), hdr.size);
  }

  this->width = ntohs( ((player_camera_data_t*)buffer)->width);
  this->height = ntohs( ((player_camera_data_t*)buffer)->height);

  this->depth = ntohs( ((player_camera_data_t*)buffer)->depth);
  this->imageSize = ntohs( ((player_camera_data_t*)buffer)->image_size);

  memcpy(this->image, ((player_camera_data_t*)buffer)->image, this->imageSize);
}

