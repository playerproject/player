/*=========================================================================
    capturev4l.cc
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
  =========================================================================*/

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if HAVE_V4L

#include "capturev4l.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>


void yuv420p_to_yuyv(unsigned char *image, unsigned char *temp, int x, int y)
{
  const int numpix = x * y;
  int i, j;
  //int y00, y01, y10, y11, u, v;
  unsigned char *pY = image;
  unsigned char *pU = pY + numpix;
  unsigned char *pV = pU + numpix / 4;
  unsigned char *image2 = temp;

  for (j = 0; j <= y - 2; j += 2) {
    for (i = 0; i <= x - 2; i += 2) {
      image2[1]=*pY;//y00;
      image2[0]=*pU;//u;
      image2[3]=*(pY+1);
      image2[2]=*pV;

      image2[x*2+1]=*(pY+x);
      image2[x*2]=(*pU++);
      image2[x*2+3]=*(pY+x+1);
      image2[x*2+2]=(*pV++);

      pY += 2;
      image2 += 4;
    }
    pY += x;
    image2 += x*2;
  }
}

//==== Capture Class Implementation =======================================//

bool capturev4l::initialize(char *device,int nwidth,int nheight,int nfmt)
{
  struct video_picture vid_pic;
  struct video_window win;
  struct video_mbuf buf;
  //struct video_mmap vid_map;
  
  //unsigned char *pic;

  int err;
  //int i;

  // Set defaults if not given
  if(!device) device = DEFAULT_VIDEO_DEVICE;
  if(!nfmt) nfmt = DEFAULT_VIDEO_FORMAT;
  if(!nwidth || !nheight){
    nwidth  = DEFAULT_IMAGE_WIDTH;
    nheight = DEFAULT_IMAGE_HEIGHT;
  }

  if (nfmt != (DEFAULT_VIDEO_FORMAT | VIDEO_PALETTE_YUV420P)) {
    printf("Error, Only YUV420 planar supported for now, sorry.\n");
    return false;
  }

  // Open the video device
  vid_fd = open(device, O_RDONLY);
  if(vid_fd == -1){
    printf("Could not open video device [%s]\n",device);
    return(false);
  }

  
  err = ioctl(vid_fd, VIDIOCGPICT, &vid_pic);
  if(err){
    printf("GPICT returned error %d\n",errno);
    return(false);
  }


  vid_pic.palette = nfmt; 

  // Set the capture format
  err = ioctl(vid_fd, VIDIOCSPICT, &vid_pic);
  if(err){
    printf("SPICT returned error %d\n",errno);
    return(false);
  }
  
  err = ioctl(vid_fd, VIDIOCGPICT, &vid_pic);
  if(err){
    printf("GPICT returned error %d\n",errno);
    return(false);
  }
  if (vid_pic.palette != nfmt) {
    printf("Couldn't set palette to %d\n", nfmt);
    return false;
  }
  
  err = ioctl(vid_fd, VIDIOCGWIN, &win);
  if(err) {
    printf("GWIN return error %d\n", errno);
    return false;
  }
  
  win.width=nwidth;
  win.height=nheight;
  
  err = ioctl(vid_fd, VIDIOCSWIN, &win);
  if(err) {
    printf("SWIN returned error %d\n", errno);
    return false;
  }

  err = ioctl(vid_fd, VIDIOCGWIN, &win);
  if(err) {
    printf("GWIN return error %d\n", errno);
    return false;
  }
  else if ((int)win.height != nheight || (int)win.width != nwidth) {
    printf("Couldn't set resolution (%d, %d), exiting\n", nwidth, nheight);
    return false;
  }

  // Request mmap-able capture buffers
  err = ioctl(vid_fd, VIDIOCGMBUF, &buf);
  if (err) {
    printf("GMBUF returned error %d\n", errno);
    return false;
  }

  
  /*
    pic = (unsigned char *)mmap (0, buf.size, PROT_READ | PROT_WRITE, MAP_PRIVATE, vid_fd, 0);
    
    if ((unsigned char *) -1 == (unsigned char *) pic) {
    printf("Unable to capture image.");
    return false;
    }
    
    vid_map.height = y;
    vid_map.width = x;
    vid_map.format = vid_pic.palette;
    for (frame = 0; frame < buf.frames; frame++) {
    vid_map.frame = frame;
    if (ioctl (vid_fd, VIDIOCMCAPTURE, &vid_map) < 0) {
    printf ("Unable to capture image (VIDIOCMCAPTURE).\n");
    return false;
    }
    }
    frame = 0;
  */
  width   = nwidth;
  height  = nheight;
  size = (width*height*3)/2;

  current = new unsigned char[width*height*2];
  temp = new unsigned char[size];
  
  return(true);
}

void capturev4l::close()
{
  /*
    int i,t;
q
    if(vid_fd >= 0){
    for(i=0; i<STREAMBUFS; i++){
    if(vimage[i].data){
    munmap(vimage[i].data,vimage[i].vidbuf.length);
    }
    }
    }
  */
}


/*
  unsigned char *capturev4l::captureFrame(int &index)
{
  return captureFrame();

  //if(captured_frame){
  //  printf("there may be a problem capturing frame w/o releasing previous frame");
  //}
  // struct v4l2_buffer tempbuf;
    int err;

    fd_set          rdset;
    struct timeval  timeout;
    int		  n;
    struct v4l2_buffer tempbuf;
    
  FD_ZERO(&rdset);
  FD_SET(vid_fd, &rdset);
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  n = select(vid_fd + 1, &rdset, NULL, NULL, &timeout);
  err = -1;
  if (n == -1) {
    fprintf(stderr, "select error.\n");
    perror("select msg:");
  }
  else if (n == 0)
    fprintf(stderr, "select timeout\n");
  else if (FD_ISSET(vid_fd, &rdset))
    err = 0;
  if(err) return(NULL);

  // Grab last frame
  do {
    //printf("D");
    tempbuf.type = vimage[0].vidbuf.type;
    err = ioctl(vid_fd, VIDIOC_DQBUF, &tempbuf);
    if(err) {
      printf("4DQBUF returned error %d\n",errno);
      perror("DQBUF error:");
    }
    
    //printf("S");
    FD_ZERO(&rdset);
    FD_SET(vid_fd, &rdset);
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    n = select(vid_fd + 1, &rdset, NULL, NULL, &timeout);
    // Uncomment this next line to see how many frames are being skipped.
    // 1=frame skipped, 0=frame used
    //printf("n%d",n);
    if(n==-1) {
      fprintf(stderr, "select error.\n");
      perror("2select msg:");
    }
    else if(n==0)
      break;
    if (!FD_ISSET(vid_fd, &rdset))
      printf("huh\n");
    
    //printf("Q");
    err = ioctl(vid_fd, VIDIOC_QBUF, &tempbuf);
    if(err) {
      printf("3QBUF returned error %d\n",errno);
      perror("QBUF error:");
    }
    
  } while(true);
    

  // Set current to point to captured frame data
  current = (unsigned char *)vimage[tempbuf.index].data;
  timestamp = tempbuf.timestamp;
  index = tempbuf.index;

  // Initiate the next capture
  //tempbuf.index = (++index - 1) % STREAMBUFS;
  //err = ioctl(vid_fd, VIDIOC_QBUF, &tempbuf);
  //if(err) printf("QBUF returned error %d\n",errno);  

  captured_frame = true;
  return(current);
}
*/


/*
void capturev4l::releaseFrame(unsigned char* frame, int index) {
    int err;
    struct v4l2_buffer tempbuf;
    //if(frame != current){
    //  printf("frame != current, possibly releasing the wrong frame.");
    //}
    captured_frame = false;
    // Initiate the next capture
    tempbuf.type = vimage[0].vidbuf.type;
    tempbuf.index=index;
    //printf("%lx %d\n",tempbuf.type,tempbuf.index);
    err = ioctl(vid_fd, VIDIOC_QBUF, &tempbuf);
    if(err) {
    printf("2QBUF returned error %d\n",errno);
    perror("QBUF error:");
    }
}
*/

unsigned char *capturev4l::captureFrame()
{
  read (vid_fd, temp, size);
  yuv420p_to_yuyv(temp, current, width, height);
  
  return current;

  // struct v4l2_buffer tempbuf;
  /*
    int err;

    fd_set          rdset;
    struct timeval  timeout;
    int		  n;
    
    FD_ZERO(&rdset);
    FD_SET(vid_fd, &rdset);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    n = select(vid_fd + 1, &rdset, NULL, NULL, &timeout);
    err = -1;
    if (n == -1)
    fprintf(stderr, "select error.\n");
    else if (n == 0)
    fprintf(stderr, "select timeout\n");
    else if (FD_ISSET(vid_fd, &rdset))
    err = 0;
    if(err) return(NULL);
    
    // Grab last frame
    tempbuf.type = vimage[0].vidbuf.type;
    err = ioctl(vid_fd, VIDIOC_DQBUF, &tempbuf);
    if(err) printf("DQBUF returned error %d\n",errno);
    
    // Set current to point to captured frame data
    current = (unsigned char *)vimage[tempbuf.index].data;
    timestamp = tempbuf.timestamp;
    
    // Initiate the next capture
    err = ioctl(vid_fd, VIDIOC_QBUF, &tempbuf);
    if(err) printf("QBUF returned error %d\n",errno);
    
    return(current);
  */
}

#endif // HAVE_V4l
