#ifndef __CAPTURECAMERA_H__
#define __CAPTURECAMERA_H__

// Desc: Capture method for using CMVision with the player camera interface 
// Author: Ben Grocholsky
// Date: 24 Feb 2004
// CVS: $Id$

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "capture.h"
#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

class captureCamera : public capture
{

/*************************
    CAMERA VARIABLES
*************************/
   private: 
    // camera device info
    Driver *camera;
    player_device_id_t camera_id;
    bool camera_open;
	
    ClientDataInternal * BaseClient;
    int camera_index;
     int width,height,depth,image_size;
     //unsigned char * current_rgb;
     unsigned char * YUV;
     player_camera_data_t data;
     double camera_time;
  
 public: 
     captureCamera(int camera_index);
     virtual ~captureCamera(); 

     virtual bool initialize(int nwidth,int nheight);
     unsigned char *captureFrame();
     unsigned char *convertImageRgb2Yuv422(unsigned char *RGB,int NumPixels);
     virtual void close(){};
     // P2CMV needs to be able to get the image geometry
     int Width(){return(width);}
     int Height(){return(height);}
};

#endif
