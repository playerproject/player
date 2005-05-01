/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Ben Grocholsky
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

// Desc: Capture method for using CMVision with the player camera interface 
// Author: Ben Grocholsky
// Date: 24 Feb 2004
// CVS: $Id$

#if HAVE_CONFIG_H
  #include "config.h"
#endif

#include <unistd.h> // for debugging file writes
#include <fcntl.h>  // for debugging file writes 
#include <netinet/in.h>
#include <stddef.h>

#include "error.h"
#include "conversions.h"
#include "captureCamera.h"
#include "playerpacket.h"

extern int global_playerport;

captureCamera::captureCamera(int camera_index) : capture()
{
  this->camera_index = camera_index;
  this->camera = NULL;
  this->camera_time = 0;
  //     cout << "captureCamera::captureCamera() index "<<this->camera_index<<" port "<<  this->camera_port<<endl;

  // Subscribe to the camera.
  camera_id.code = PLAYER_CAMERA_CODE;
  camera_id.index = this->camera_index;
  camera_id.port = global_playerport;
  this->camera = deviceTable->GetDriver(camera_id);
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    camera_open = false;
    return;
  }
  if (this->camera->Subscribe(this->camera_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    camera_open = false;
    return;
  }

  // get an image to find it's configuration 
  char dname[64];
  // find the camera driver
  sprintf(dname,"%s",deviceTable->GetDriverName(camera_id));
  // gz_camera will lock up if you do this. Others Need it.
  if (strcmp(dname,"gz_camera"))
       this->camera->Wait();

  size_t size;
  struct timeval timestamp;
  this->camera->Update();
  // Get the camera data.
  size = this->camera->GetData(this->camera_id, (unsigned char*) &data,
			       sizeof(data), &timestamp);

  // Do some byte swapping
  this->width = ntohs(data.width);
  this->height = ntohs(data.height); 
  this->depth = data.bpp;
   
  this->image_size = ntohl(data.image_size); 
  /*  cout << "captureCamera::initialize()"<<endl;
      cout << "width " << width<<endl;
      cout << "height " <<height  <<endl;
      cout << "depth " <<depth  <<endl;
      cout << "image_size " <<image_size <<endl;*/
  YUV=(unsigned char *)malloc(width*height*2);
  if (!YUV)
  {
    PLAYER_ERROR("Out of memory");
    camera_open = false;
    return;
  }
 
  camera_open = true;
}

captureCamera::~captureCamera()
{
  this->camera->Unsubscribe(this->camera_id);
  if (YUV) free(YUV);
  YUV=NULL;
  current=NULL;
} 

bool captureCamera::initialize(int nwidth,int nheight)
{
     // nothing to do for Player camera Interface
     // let P2CMV know if the camera was opened
     return (camera_open);
}

unsigned char *captureCamera::captureFrame()
{
     struct timeval timestamp;
     size_t size;
     double t;
     int w,h;
     unsigned char * ptr = NULL;
#if HAVE_JPEGLIB_H
     int dst_size;
     unsigned char * dst;
#endif

     this->camera->Wait();
     size = this->camera->GetData(this->camera_id, (unsigned char*) &data,
				  sizeof(data), &timestamp);

     t = (double) timestamp.tv_sec + ((double) timestamp.tv_usec) * 1e-6;
     // Dont do anything if this is old data.
     if (0)//(fabs(t - this->camera_time) < 0.001)
	  {
	       printf("old camera data %f seconds\n",t - this->camera_time);
	       return NULL;
	  }
     this->camera_time = t;
     
     // check if image has changed size
     w = ntohs(data.width);
     h = ntohs(data.height);
     if ((w!=width)||(h!=height))
	  {
	       printf("captureCamera::captureFrame() camera resized %dx%d\n",w,h);
	       // this will lose memory but it hopefully will not happen
	       // just here 'cause it can happen
	       // can't free it as CMVisionBF::Main() might be using it
	       // free(YUV);
	       if ((w>width)||(h>height))
	       {
	    	    if (YUV) free(YUV);
		    YUV=(unsigned char *)malloc(w*h*2);
		    if (!YUV)
		    {
			PLAYER_ERROR("Out of memory");
			return NULL;
		    }
	       }
	       width = w;
	       height = h;
	  }
     switch (data.compression)
     {
     case PLAYER_CAMERA_COMPRESS_RAW:
        ptr = convertImageRgb2Yuv422(data.image, width * height);
        break;
     case PLAYER_CAMERA_COMPRESS_JPEG:
#if HAVE_JPEGLIB_H
	// Create a temp buffer
	dst_size = width * height * (data.bpp / 8);
	dst = (unsigned char *)malloc(dst_size);

	// Decompress into temp buffer
	jpeg_decompress(dst, dst_size, data.image, ntohl(data.image_size));

	ptr = convertImageRgb2Yuv422(dst, width * height);
	free(dst);
#else
	PLAYER_ERROR("JPEG decompression support was not included at compile-time");
	ptr = NULL;
#endif
	break;
     default:
	PLAYER_ERROR("Unknown compression type");
	ptr = NULL;        
     }
     return ptr;
}


unsigned char * captureCamera::convertImageRgb2Yuv422(unsigned char *RGB,int NumPixels)
{
     
     // use 2001 version of conversions.h from Dan Dennedy  <dan@dennedy.org>
     //rgb2yuy2(RGB, YUV, NumPixels);
     // use latest conversions.h from coriander 
     rgb2uyvy(RGB, YUV, NumPixels);
     return YUV;
}

