#ifndef __CAPTURE1394_H__
#define __CAPTURE1394_H__

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <libraw1394/raw1394.h>
#include <libdc1394/dc1394_control.h>

#include "capture.h"

#define DEFAULT_IMAGE_WIDTH   320
#define DEFAULT_IMAGE_HEIGHT  240
//we only have one buffer that I know of...
#define STREAMBUFS            1

class capture1394 : public capture
{

/*************************
    CAMERA VARIABLES
*************************/
  dc1394_cameracapture camera;
  int numNodes;
  int numCameras;
  raw1394handle_t handle;
  nodeid_t * camera_nodes;

public:
  capture1394():capture() {camera_nodes=NULL;}
  virtual ~capture1394() {close();}

  virtual bool initialize(int nwidth,int nheight);
  virtual void close();
  virtual unsigned char *captureFrame();

  // are these needed?
  /*
  bool initialize(char *device,int nwidth,int nheight,int nfmt)
  { return initialize(nwidth,nheight); }
  bool initialize()
  { return initialize(0,0); }
  unsigned char *captureFrame(int &index,int &field);
  void releaseFrame(unsigned char* frame, int index);

  unsigned char *getFrame() {return(current);}
  stamp_t getFrameTime() {return(timestamp);}
  double getFrameTimeSec() {return(timestamp * 1.0E-9);}
  int getWidth() {return(width);}
  int getHeight() {return(height);}
  */
};

#endif // __CAPTURE1394_H__
