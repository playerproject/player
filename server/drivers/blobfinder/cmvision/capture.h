#ifndef __CAPTURE_H__
#define __CAPTURE_H__

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

typedef long long stamp_t;

#define DEFAULT_IMAGE_WIDTH   320
#define DEFAULT_IMAGE_HEIGHT  240
//we only have one buffer that I know of...
#define STREAMBUFS            1

class capture {

/*************************
    CAMERA VARIABLES
*************************/
  dc1394_cameracapture camera;
  int numNodes;
  int numCameras;
  raw1394handle_t handle;
  nodeid_t * camera_nodes;

  //Capture class variables
  unsigned char *current; // most recently captured frame
  stamp_t timestamp;      // frame time stamp
  int width,height;       // dimensions of video frame
  bool captured_frame;
public:
  capture() {camera_nodes=NULL; current=NULL; captured_frame = false;}
  ~capture() {close();}

  bool initialize(char *device,int nwidth,int nheight,int nfmt)
  { return initialize(nwidth,nheight); }
  bool initialize(int nwidth,int nheight);
  bool initialize()
  { return initialize(0,0); }

  void close();

  unsigned char *captureFrame(int &index,int &field);
  unsigned char *captureFrame();
  void releaseFrame(unsigned char* frame, int index);

  unsigned char *getFrame() {return(current);}
  stamp_t getFrameTime() {return(timestamp);}
  double getFrameTimeSec() {return(timestamp * 1.0E-9);}
  int getWidth() {return(width);}
  int getHeight() {return(height);}
};

#endif // __CAPTURE_H__
