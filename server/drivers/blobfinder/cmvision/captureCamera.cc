#if HAVE_CONFIG_H
  #include <config.h>
#endif

// Desc: Capture method for using CMVision with the player camera interface 
// Author: Ben Grocholsky
// Date: 24 Feb 2004
// CVS: $Id$

#include "conversions.h"
#include "captureCamera.h"

#include <unistd.h> // for debugging file writes
#include <fcntl.h>  // for debugging file writes 

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
  //this->camera->Wait();
  size_t size;
  struct timeval timestamp;
  this->camera->Update();
  // Get the camera data.
  size = this->camera->GetData(this->camera_id, (unsigned char*) &data,
			       sizeof(data), &timestamp);

  // Do some byte swapping
  this->width = ntohs(data.width);
  this->height = ntohs(data.height); 
  this->depth = data.depth;

    
  this->image_size = ntohl(data.image_size); 
  /*  cout << "captureCamera::initialize()"<<endl;
      cout << "width " << width<<endl;
      cout << "height " <<height  <<endl;
      cout << "depth " <<depth  <<endl;
      cout << "image_size " <<image_size <<endl;*/
  YUV=(unsigned char *)malloc(width*height*2);
  camera_open = true;
}

captureCamera::~captureCamera()
{
  this->camera->Subscribe(this->camera_id);
  free(YUV);
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

     //this->camera->Update(); //?
     this->camera->Wait();
     size = this->camera->GetData(this->camera_id, (unsigned char*) &data,
				  sizeof(data), &timestamp);
     t = (double) timestamp.tv_sec + ((double) timestamp.tv_usec) * 1e-6;
     // Dont do anything if this is old data.
     if (0)//(fabs(t - this->camera_time) < 0.001)
	  {
	       printf("old camera data %f seconds\n",t - this->camera_time);
	       return 0;
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
		    YUV=(unsigned char *)malloc(w*h*2);
	       width = w;
	       height = h;
	  }
     return convertImageRgb2Yuv422(data.image,width * height);
}


unsigned char * captureCamera::convertImageRgb2Yuv422(unsigned char *RGB,int NumPixels)
{
     
     // use 2001 version of conversions.h from Dan Dennedy  <dan@dennedy.org>
     //rgb2yuy2(RGB, YUV, NumPixels);
     // use latest conversions.h from coriander 
     rgb2uyvy(RGB, YUV, NumPixels);
     return YUV;
}

