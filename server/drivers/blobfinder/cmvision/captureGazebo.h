#ifndef __CAPTUREGAZEBO_H__
#define __CAPTUREGAZEBO_H__

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "capture.h"
#include "gazebo.h"
#include "gz_camera.h"

#define DEFAULT_IMAGE_WIDTH   320
#define DEFAULT_IMAGE_HEIGHT  240
//we only have one buffer that I know of...
#define STREAMBUFS            1

extern CDevice * CMGzCamera_Init(char *interface, ConfigFile *cf, int section);

class captureGazebo : public capture
{

/*************************
    CAMERA VARIABLES
*************************/
  CMGzCamera *camera;
  int width,height;
  unsigned char * current_rgb;
  unsigned char * YUV;
public: 
 captureGazebo(char *interface, ConfigFile *cf, int section):capture()   
 {     camera= (CMGzCamera *)CMGzCamera_Init(interface,cf,section);
       // printf("Got cam\n"); It gets through the constructor!
       camera->Setup();
       
    }

  virtual ~captureGazebo() 
{
    camera->Shutdown();
    close();}

  virtual bool initialize(int nwidth,int nheight);
  virtual void close();
  unsigned char *captureFrame();
  unsigned char *  convertImageRgb2Yuv422(unsigned char *RGB,int NumPixels);
  // image_pixel * uchar2impixel(unsigned char * image,int NumPixels);

};

#endif
