/*=========================================================================
    capture.h
  -------------------------------------------------------------------------
    Example code for video capture under Video4Linux II
  -------------------------------------------------------------------------
    Copyright 1999, 2000
    Anna Helena Reali Costa, James R. Bruce
    School of Computer Science
    Carnegie Mellon University
  -------------------------------------------------------------------------
    This source code is distributed "as is" with absolutely no warranty.
    It is covered under the GNU General Public Licence, Version 2.0;
    See COPYING, which should be included with this distribution.
  -------------------------------------------------------------------------
    Revision History:
      2003-05-08:  Modified to be included as a driver in Player (gerkey)
      2000-02-05:  Ported to work with V4L2 API
      1999-11-23:  Quick C++ port to simplify & wrap in an object (jbruce) 
      1999-05-01:  Initial version (annar)
  =========================================================================*/

#ifndef __CAPTUREV4L2_H__
#define __CAPTUREV4L2_H__

#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/videodev.h>

#define DEFAULT_VIDEO_DEVICE  "/dev/video"
#define VIDEO_STANDARD        "NTSC"

#ifdef USE_METEOR
#define DEFAULT_VIDEO_FORMAT  V4L2_PIX_FMT_UYVY
#else
#define DEFAULT_VIDEO_FORMAT  V4L2_PIX_FMT_YUYV
#endif

#define DEFAULT_IMAGE_WIDTH   320
#define DEFAULT_IMAGE_HEIGHT  240
// if you get a message like "DQBUF returned error", "DQBUF error: invalid"
// then you need to use a higher value for STREAMBUFS or process frames faster
#define STREAMBUFS            4

class captureV4L2
{
  private:
    struct vimage_t {
      v4l2_buffer vidbuf;
      char *data;
    };

    int vid_fd;                    // video device
    vimage_t vimage[STREAMBUFS];   // buffers for images
    struct v4l2_format fmt;        // video format request
    struct v4l2_buffer tempbuf;

  public:
    captureV4L2() : capture() {vid_fd = 0;}
    virtual ~captureV4L2() {close();}

    virtual unsigned char *captureFrame();
    virtual void close();
    virtual bool initialize(int nwidth,int nheight)
    {return(initialize(NULL,nwidth,nheight,0));}

    bool initialize(char *device,int nwidth,int nheight,int nfmt);

  /*
  bool initialize()
    {return(initialize(NULL,0,0,0));}
  unsigned char *captureFrame(int &index,int &field);
  void releaseFrame(unsigned char* frame, int index);
  unsigned char *getFrame() {return(current);}
  stamp_t getFrameTime() {return(timestamp);}
  double getFrameTimeSec() {return(timestamp * 1.0E-9);}
  int getWidth() {return(width);}
  int getHeight() {return(height);}
  */
};

#endif // __CAPTUREV4L2_H__
