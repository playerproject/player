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
#include "driver.h"

class ClientDataInternal;

class captureCamera : public capture, public Driver
{

/*************************
    CAMERA VARIABLES
*************************/
   private: 
    // camera device info
    Driver *camera;
    player_device_id_t camera_id;
    bool camera_open;
	bool NewCamData;
	
    ClientDataInternal * BaseClient;
    int camera_index;
     int width,height,depth,image_size;
     //unsigned char * current_rgb;
     unsigned char * YUV, *FrameData;;
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

     int Setup() {return 0;};
     int Shutdown() {return 0;};

  // Process incoming messages from clients 
  int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, int * resp_len);
     
};

#endif
