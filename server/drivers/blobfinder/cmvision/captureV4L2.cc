#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if HAVE_V4L2

/*=========================================================================
    capture.cc
  -------------------------------------------------------------------------
    Example code for video capture under Video4Linux II
  -------------------------------------------------------------------------
    Copyright 1999, 2000
    Anna Helena Reali Costa, James R. Bruce
    School of Computer Science
    Carnegie Mellon University
  -------------------------------------------------------------------------
    This source code is distributed "as is" with absolutely no warranty.
    See LICENSE, which should be included with this distribution.
  -------------------------------------------------------------------------
    Revision History:
      2000-02-05:  Ported to work with V4L2 API
      1999-11-23:  Quick C++ port to simplify & wrap in an object (jbruce) 
      1999-05-01:  Initial version (annar)
  =========================================================================*/

#include "captureV4L2.h"


//==== Capture Class Implementation =======================================//

void grabSetFps(int fd, int fps)
{
  struct v4l2_streamparm params;

  //printf("called v4l2_set_fps with fps=%d\n",fps);
  params.type = V4L2_BUF_TYPE_CAPTURE;
  ioctl(fd, VIDIOC_G_PARM, &params);
  //printf("time per frame is: %ld\n", params.parm.capture.timeperframe);
  params.parm.capture.capturemode |= V4L2_CAP_TIMEPERFRAME;
  params.parm.capture.timeperframe = 10000000 / fps;
  if (fps == 30)
    params.parm.capture.timeperframe = 333667;
  // printf("time per frame is: %ld\n", params.parm.capture.timeperframe);
  ioctl(fd, VIDIOC_S_PARM, &params);
  
  params.parm.capture.timeperframe = 0;
  ioctl(fd, VIDIOC_G_PARM, &params);
  // printf("time per frame is: %ld\n", params.parm.capture.timeperframe);  
}

bool captureV4L2::initialize(char *device,int nwidth,int nheight,int nfmt)
{
  struct v4l2_requestbuffers req;
  int err;
  int i;

  // Set defaults if not given
  if(!device) device = DEFAULT_VIDEO_DEVICE;
  if(!nfmt) nfmt = DEFAULT_VIDEO_FORMAT;
  if(!nwidth || !nheight){
    nwidth  = DEFAULT_IMAGE_WIDTH;
    nheight = DEFAULT_IMAGE_HEIGHT;
  }

  // Open the video device
  vid_fd = open(device, O_RDONLY);
  if(vid_fd == -1){
    printf("Could not open video device [%s]\n",device);
    return(false);
  }

  fmt.type = V4L2_BUF_TYPE_CAPTURE;
  err = ioctl(vid_fd, VIDIOC_G_FMT, &fmt);
  if(err){
    printf("G_FMT returned error %d\n",errno);
    return(false);
  }

  // Set video format
  fmt.fmt.pix.width = nwidth;
  fmt.fmt.pix.height = nheight;
  fmt.fmt.pix.pixelformat = nfmt;

  if(true){
    // attempt double framerate
    fmt.fmt.pix.flags = V4L2_FMT_FLAG_TOPFIELD; // |V4L2_FMT_FLAG_BOTFIELD;
    /*
    fmt.fmt.pix.flags = (fmt.fmt.pix.flags |
			 V4L2_FMT_FLAG_TOPFIELD | V4L2_FMT_FLAG_BOTFIELD) &
                         ~V4L2_FMT_FLAG_INTERLACED;
    */
  }else{
    fmt.fmt.pix.flags = fmt.fmt.pix.flags | V4L2_FMT_FLAG_INTERLACED;
  }

  err = ioctl(vid_fd, VIDIOC_S_FMT, &fmt);
  if(err){
    printf("S_FMT returned error %d\n",errno);
    perror("S_FMT:");
    return(false);
  }

  // grabSetFps(vid_fd, 30);

  // Request mmap-able capture buffers
  req.count = STREAMBUFS;
  req.type  = V4L2_BUF_TYPE_CAPTURE;
  err = ioctl(vid_fd, VIDIOC_REQBUFS, &req);
  if(err < 0 || req.count < 1){
    printf("REQBUFS returned error %d, count %d\n",
	   errno,req.count);
    return(false);
  }

  for(i=0; i<req.count; i++){
    vimage[i].vidbuf.index = i;
    vimage[i].vidbuf.type = V4L2_BUF_TYPE_CAPTURE;
    err = ioctl(vid_fd, VIDIOC_QUERYBUF, &vimage[i].vidbuf);
    if(err < 0){
      printf("QUERYBUF returned error %d\n",errno);
      return(false);
    }

    vimage[i].data = (char*)mmap(0, vimage[i].vidbuf.length, PROT_READ,
			  MAP_SHARED, vid_fd, 
			  vimage[i].vidbuf.offset);
    if((int)vimage[i].data == -1){
      printf("mmap() returned error %d\n", errno);
      return(false);
    }
  }

  for(i=0; i<req.count; i++){
    if((err = ioctl(vid_fd, VIDIOC_QBUF, &vimage[i].vidbuf))){
      printf("1QBUF returned error %d\n",errno);
      perror("QBUF error");
      return(false);
    }
  }

  // Turn on streaming capture
  err = ioctl(vid_fd, VIDIOC_STREAMON, &vimage[0].vidbuf.type);
  if(err){
    printf("STREAMON returned error %d\n",errno);
    return(false);
  }

  width   = nwidth;
  height  = nheight;
  current = NULL;

  return(true);
}

void captureV4L2::close()
{
  int i,t;

  if(vid_fd >= 0){
    t = V4L2_BUF_TYPE_CAPTURE;
    ioctl(vid_fd, VIDIOC_STREAMOFF, &t);

    for(i=0; i<STREAMBUFS; i++){
      if(vimage[i].data){
        munmap(vimage[i].data,vimage[i].vidbuf.length);
      }
    }
  }
}


/*
unsigned char *capture::captureFrame(int &index,int &field)
{
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

  field = 0;
  if(tempbuf.flags & V4L2_BUF_FLAG_TOPFIELD) field = 1;
  if(tempbuf.flags & V4L2_BUF_FLAG_BOTFIELD) field = 0;

  // Set current to point to captured frame data
  current = (unsigned char *)vimage[tempbuf.index].data;
  timestamp = tempbuf.timestamp;
  index = tempbuf.index;

  // gettimeofday(&timeout,NULL);
  // timestamp = (stamp_t)((timeout.tv_sec + timeout.tv_usec/1.0E6) * 1.0E9);
  // printf("field: %d\n",field);

  // Initiate the next capture
  //tempbuf.index = (++index - 1) % STREAMBUFS;
  //err = ioctl(vid_fd, VIDIOC_QBUF, &tempbuf);
  //if(err) printf("QBUF returned error %d\n",errno);  

  captured_frame = true;
  return(current);
}
*/

/*
void capture::releaseFrame(unsigned char* frame, int index)
{
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

struct v4l2_buffer tempbuf;

unsigned char *captureV4L2::captureFrame()
{
  // struct v4l2_buffer tempbuf;
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
}

#endif // HAVE_V4L2
