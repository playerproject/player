/*
  -------------------------------------------------------------------------
   capturev4l.h
  -------------------------------------------------------------------------
   Capture code for the original V4L (video for linux) API
   (c) 2003 Brendan Burns (bburns@cs.umass.edu)
  -------------------------------------------------------------------------
   Heavily adapted from the capturev4l2.cc code:
   Copyright 1999, 2000 Anna Helena Reali Costa, James R. Bruce, CMU
  -------------------------------------------------------------------------
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
    
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
  -------------------------------------------------------------------------
    Revision History:
      10-30-2003 Initial Version
*/
#ifndef __CAPTURE_V4L__
#define __CAPTURE_V4L__

#include "capture.h"

#define DEFAULT_VIDEO_DEVICE "/dev/video0"
// This is the default for Phillips WebCams...
#define DEFAULT_VIDEO_FORMAT VIDEO_PALETTE_YUV420P
#define DEFAULT_IMAGE_WIDTH 320
#define DEFAULT_IMAGE_HEIGHT 240


class capturev4l : public capture {
  int vid_fd;
  unsigned char* temp;
  int width, height;
  int size;
 
public:
  capturev4l() {vid_fd = 0; current=0; captured_frame = false;}
  virtual ~capturev4l() {close();}

  bool initialize(char *device,int nwidth,int nheight,int nfmt);
  virtual bool initialize(int nwidth,int nheight)
    {return(initialize(0,nwidth,nheight,0));}
  bool initialize()
    {return(initialize(0,0,0,0));}

  virtual void close();

  virtual unsigned char *captureFrame();
  void releaseFrame(unsigned char* frame, int index);

  unsigned char *getFrame() {return(current);}
  // stamp_t getFrameTime() {return(timestamp);}
  int getWidth() {return(width);}
  int getHeight() {return(height);}
};
#endif
