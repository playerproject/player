/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "v4l2.h"
#include "bayer.h"
#include <unistd.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#define v4l2_fmtbyname(name) v4l2_fourcc((name)[0], (name)[1], (name)[2], (name)[3])

#define FAIL -1

#define REQUEST_BUFFERS 4

struct fg_struct
{
 int dev_fd;
 int grabbing;
 int grab_number;
 int depth;
 int buffers_num;
 unsigned int pixformat;
 int r, g, b;
 struct buff_struct
 {
  struct v4l2_buffer buffer;
  unsigned char * video_map;
 } buffers[REQUEST_BUFFERS];
 int width;
 int height;
 int pixels;
 int imgdepth;
 unsigned char * bayerbuf;
 int bayerbuf_size;
 unsigned char * image;
};

#define FG(ptr) ((struct fg_struct *)(ptr))

int fg_width(void * fg)
{
 return FG(fg)->width;
}

int fg_height(void * fg)
{
 return FG(fg)->height;
}

int fg_grabdepth(void * fg)
{
 return FG(fg)->depth;
}

int fg_imgdepth(void * fg)
{
 return FG(fg)->imgdepth;
}

int set_channel(void * fg, int channel, const char * mode)
{
 int m;

 if (FG(fg)->grabbing) return FAIL;
 if (channel < 0) return FAIL;
 if (ioctl (FG(fg)->dev_fd, VIDIOC_S_INPUT, &channel) == -1)
 {
  fprintf(stderr, "ioctl error (VIDIOC_S_INPUT)\n");
  return FAIL;
 }
 if (!strcmp(mode, "UNKNOWN")) m = 0;
 else if (!strcmp(mode, "PAL")) m = V4L2_STD_PAL;
 else if (!strcmp(mode, "NTSC")) m = V4L2_STD_NTSC;
 else
 {
  fprintf(stderr, "invalid mode %s\n", mode);
  return FAIL;
 }
 if (m > 0)
 {
  if (ioctl (FG(fg)->dev_fd, VIDIOC_S_STD, &m) == -1)
  {
   fprintf(stderr, "ioctl error (VIDIOC_S_STD)\n");
   return FAIL;
  }
 }
 return 0;
}

int start_grab(void * fg)
{
 int i;
 enum v4l2_buf_type type;

 if (FG(fg)->grabbing) return 0;
 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 ioctl(FG(fg)->dev_fd, VIDIOC_STREAMOFF, &type);
 for (i = 0; i < (FG(fg)->buffers_num); i++)
 {
  if (ioctl(FG(fg)->dev_fd, VIDIOC_QBUF, &(FG(fg)->buffers[i].buffer)) == -1)
  {
   fprintf(stderr, "ioctl error (VIDIOC_QBUF)\n");
   type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   ioctl(FG(fg)->dev_fd, VIDIOC_STREAMOFF, &type);
   return FAIL;
  }
 }
 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 if (ioctl(FG(fg)->dev_fd, VIDIOC_STREAMON, &type) == -1)
 {
  fprintf(stderr, "ioctl error (VIDIOC_STREAMON)\n");
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  ioctl(FG(fg)->dev_fd, VIDIOC_STREAMOFF, &type);
  return FAIL;
 }
 FG(fg)->grab_number = 0;
 FG(fg)->grabbing = !0;
 return 0;
}

void stop_grab(void * fg)
{
 enum v4l2_buf_type type;

 if (FG(fg)->grabbing)
 {
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (ioctl(FG(fg)->dev_fd, VIDIOC_STREAMOFF, &type) == -1)
  {
   fprintf(stderr, "ioctl error (VIDIOC_STREAMOFF)\n");
  }
  FG(fg)->grabbing = 0;
 }
}

unsigned char * get_image(void * fg)
{
 enum v4l2_buf_type type;
 int i, grabdepth;
 const unsigned char * buf;
 unsigned char * img;
 int count, insize;
 unsigned char table5[] = { 0, 8, 16, 25, 33, 41, 49,  58, 66, 74, 82, 90, 99, 107, 115, 123, 132, 140, 148, 156, 165, 173, 181, 189,  197, 206, 214, 222, 230, 239, 247, 255 };
 unsigned char table6[] = { 0, 4, 8, 12, 16, 20, 24, 28, 32, 36, 40, 45, 49, 53, 57, 61, 65, 69, 73, 77, 81, 85, 89, 93, 97, 101,  105, 109, 113, 117, 121, 125, 130, 134, 138, 142, 146, 150, 154, 158, 162, 166, 170, 174, 178, 182, 186, 190, 194, 198, 202, 206, 210, 215, 219, 223, 227, 231, 235, 239, 243, 247, 251, 255 };

 if ((!(FG(fg)->grabbing)) || (!(FG(fg)->image))) return NULL;
 memset(&(FG(fg)->buffers[FG(fg)->grab_number].buffer), 0, sizeof FG(fg)->buffers[FG(fg)->grab_number].buffer);
 FG(fg)->buffers[FG(fg)->grab_number].buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 FG(fg)->buffers[FG(fg)->grab_number].buffer.memory = V4L2_MEMORY_MMAP;
 if (ioctl(FG(fg)->dev_fd, VIDIOC_DQBUF, &(FG(fg)->buffers[FG(fg)->grab_number].buffer)) == -1)
 {
  fprintf(stderr, "get_image: ioctl error (VIDIOC_DQBUF)\n");
  return NULL;
 }
 buf = FG(fg)->buffers[FG(fg)->grab_number].video_map;
 if (!buf)
 {
  fprintf(stderr, "NULL buffer pointer\n");
  return NULL;
 }
 grabdepth = FG(fg)->depth;
 if ((FG(fg)->pixformat) == v4l2_fmtbyname("BA81"))
 {
  if (!(FG(fg)->bayerbuf)) return NULL;
  bayer2rgb24(FG(fg)->bayerbuf, FG(fg)->buffers[FG(fg)->grab_number].video_map, FG(fg)->width, FG(fg)->height);
  buf = FG(fg)->bayerbuf;
  grabdepth = 3;
 } else if ((FG(fg)->pixformat) == v4l2_fmtbyname("RGBP"))
 {
  if (!(FG(fg)->bayerbuf)) return NULL;
  img = FG(fg)->bayerbuf;
  for (i = 0; i < (FG(fg)->pixels); i++)
  {
    img[0] = table5[(buf[1]) >> 3];
    img[1] = table6[(((buf[1]) & 7) << 3) | ((buf[0]) >> 5)];
    img[2] = table5[(buf[0]) & 0xe0];
    img += 3; buf += 2;
  }
  buf = FG(fg)->bayerbuf;
  grabdepth = 3;
 }
 img = FG(fg)->image;
 if ((FG(fg)->pixformat) == v4l2_fmtbyname("MJPG"))
 { /* taken directly from v4lcapture.c (camerav4l driver code) */
  insize = ((FG(fg)->pixels) * grabdepth) - sizeof(int);
  count = insize - 1;
  for (i = 1024; i < count; i++)
  {
   if (buf[i] == 0xff) if (buf[i + 1] == 0xd9)
   {
    insize = i + 10;
    break;
   }
  }
  if (insize > 1)
  {
    memcpy(img, &insize, sizeof(int));
    memcpy(img + sizeof(int), buf, insize);
  } else
  {
    fprintf(stderr, "Internal error\n");
    return NULL;
  }
 } else for (i = 0; i < (FG(fg)->pixels); i++)
 {
  switch(FG(fg)->imgdepth)
  {
  case 1:
   switch(grabdepth)
   {
   case 1:
    img[0] = buf[0];
    img++; buf++;
    break;
   case 3:
    img[0] = (unsigned char)((buf[FG(fg)->r] * 0.3) + (buf[FG(fg)->g] * 0.59) + (buf[FG(fg)->b] * 0.11));
    img++; buf += 3;
    break;
   case 4:
    img[0] = (unsigned char)((buf[FG(fg)->r] * 0.3) + (buf[FG(fg)->g] * 0.59) + (buf[FG(fg)->b] * 0.11));
    img++; buf += 4;
    break;
   }
   break;
  case 3:
   switch(grabdepth)
   {
   case 1:
    img[0] = buf[0];
    img[1] = buf[0];
    img[2] = buf[0];
    img += 3; buf++;
    break;
   case 3:
    img[0] = buf[FG(fg)->r];
    img[1] = buf[FG(fg)->g];
    img[2] = buf[FG(fg)->b];
    img += 3; buf += 3;
    break;
   case 4:
    img[0] = buf[FG(fg)->r];
    img[1] = buf[FG(fg)->g];
    img[2] = buf[FG(fg)->b];
    img += 3; buf += 4;
    break;
   }
   break;
  case 4:
   switch(grabdepth)
   {
   case 1:
    img[0] = buf[0];
    img[1] = buf[0];
    img[2] = buf[0];
    img[3] = buf[0];
    img += 4; buf++;
    break;
   case 3:
    img[0] = buf[FG(fg)->r];
    img[1] = buf[FG(fg)->g];
    img[2] = buf[FG(fg)->b];
    img[3] = 0;
    img += 4; buf += 3;
    break;
   case 4:
    img[0] = buf[FG(fg)->r];
    img[1] = buf[FG(fg)->g];
    img[2] = buf[FG(fg)->b];
    img[3] = buf[3];
    img += 4; buf += 4;
    break;
   }
   break;
  }
 }
 if (ioctl(FG(fg)->dev_fd, VIDIOC_QBUF, &(FG(fg)->buffers[FG(fg)->grab_number].buffer)) == -1)
 {
  fprintf(stderr, "get_image: ioctl error (VIDIOC_QBUF)\n");
  return NULL;
 }
 type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 ioctl(FG(fg)->dev_fd, VIDIOC_STREAMON, &type);
 FG(fg)->grab_number++;
 if ((FG(fg)->grab_number) >= (FG(fg)->buffers_num)) FG(fg)->grab_number = 0;
 return FG(fg)->image;
}

void * open_fg(const char * dev, const char * pixformat, int width, int height, int imgdepth, int buffers)
{
 int i;
 struct v4l2_requestbuffers reqbuf;
 struct v4l2_format format;
 struct fg_struct * fg;

 fg = malloc(sizeof(struct fg_struct));
 if (!fg)
 {
    fprintf(stderr, "out of memory\n");
    return NULL;
 }
 fg->dev_fd = -1;
 fg->grabbing = 0;
 fg->buffers_num = 0;
 for (i = 0; i < REQUEST_BUFFERS; i++) fg->buffers[i].video_map = NULL;
 fg->image = NULL;
 fg->bayerbuf = NULL;
 fg->bayerbuf_size = 0;
 if ((width <= 0) || (height <= 0))
 {
  fprintf(stderr, "invalid image size\n");
  free(fg);
  return NULL;
 }
 fg->width = width; fg->height = height; fg->pixels = (width * height);
 if (imgdepth <= 0)
 {
  fprintf(stderr, "invalid depth given\n");
  free(fg);
  return NULL;
 }
 fg->imgdepth = imgdepth;
 if (buffers <= 0)
 {
  fprintf(stderr, "invalid number of requested buffers\n");
  free(fg);
  return NULL;
 }
 if (buffers > REQUEST_BUFFERS) buffers = REQUEST_BUFFERS;
 fg->buffers_num = buffers;
 fg->pixformat = v4l2_fmtbyname(pixformat);
 if ((fg->pixformat) == v4l2_fmtbyname("GREY"))
 {
  fg->r = 0; fg->g = 0; fg->b = 0;
  fg->depth = 1;
 } else if ((fg->pixformat) == v4l2_fmtbyname("RGBP"))
 {
  fg->depth = 2;
  fg->r = 0; fg->g = 1; fg->b = 2;
  fg->bayerbuf_size = width * height * 3;
  fg->bayerbuf = malloc(fg->bayerbuf_size);
  if (!(fg->bayerbuf))
  {
   fprintf(stderr, "out of memory\n");
   fg->bayerbuf_size = 0;
   free(fg);
   return NULL;
  }
 } else if ((fg->pixformat) == v4l2_fmtbyname("BGR3"))
 {
  fg->r = 2; fg->g = 1; fg->b = 0;
  fg->depth = 3;
 } else if ((fg->pixformat) == v4l2_fmtbyname("BGR4"))
 {
  fg->r = 2; fg->g = 1; fg->b = 0;
  fg->depth = 4;
 } else if ((fg->pixformat) == v4l2_fmtbyname("RGB3"))
 {
  fg->r = 0; fg->g = 1; fg->b = 2;
  fg->depth = 3;
 } else if ((fg->pixformat) == v4l2_fmtbyname("RGB4"))
 {
  fg->r = 0; fg->g = 1; fg->b = 2;
  fg->depth = 4;
 } else if ((fg->pixformat) == v4l2_fmtbyname("BA81"))
 {
  fg->depth = 1;
  fg->r = 0; fg->g = 1; fg->b = 2;
  fg->bayerbuf_size = width * height * 3;
  fg->bayerbuf = malloc(fg->bayerbuf_size);
  if (!(fg->bayerbuf))
  {
   fprintf(stderr, "out of memory\n");
   fg->bayerbuf_size = 0;
   free(fg);
   return NULL;
  }
 } else if ((fg->pixformat) == v4l2_fmtbyname("MJPG"))
 {
  fg->r = 0; fg->g = 0; fg->b = 0;
  fg->depth = 3;
 } else
 {
  fprintf(stderr, "unknown pixel format %s\n", pixformat);
  free(fg);
  return NULL;
 }
 fg->image = malloc(width * height * imgdepth);
 if (!(fg->image))
 {
  fprintf(stderr, "out of memory\n");
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 fg->dev_fd = open(dev, O_RDWR);
 if (fg->dev_fd == -1)
 {
  fprintf(stderr, "cannot open %s\n", dev);
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 memset(&format, 0, sizeof format);
 format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 format.fmt.pix.width = width;
 format.fmt.pix.height = height;
 format.fmt.pix.pixelformat = v4l2_fmtbyname(pixformat);
 format.fmt.pix.field = V4L2_FIELD_ANY;
 format.fmt.pix.bytesperline = 0;
 /* rest of the v4l2_pix_format structure fields are set by driver */
 if (ioctl(fg->dev_fd, VIDIOC_S_FMT, &format) == -1)
 {
  fprintf(stderr, "ioctl error (VIDIOC_S_FMT)\n");
  close(fg->dev_fd); fg->dev_fd = -1;
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 memset(&reqbuf, 0, sizeof reqbuf);
 reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 reqbuf.memory = V4L2_MEMORY_MMAP;
 reqbuf.count = (fg->buffers_num);
 if (ioctl(fg->dev_fd, VIDIOC_REQBUFS, &reqbuf) == -1)
 {
  fprintf(stderr, "ioctl error (VIDIOC_REQBUFS)\n");
  close(fg->dev_fd); fg->dev_fd = -1;
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 if ((reqbuf.count) != (fg->buffers_num))
 {
  fprintf(stderr, "reqbuf.count != fg->buffers_num\n");
  close(fg->dev_fd); fg->dev_fd = -1;
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 for (i = 0; i < (fg->buffers_num); i++)
 {
  fg->buffers[i].buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fg->buffers[i].buffer.memory = V4L2_MEMORY_MMAP;
  fg->buffers[i].buffer.index = i;
  if (ioctl(fg->dev_fd, VIDIOC_QUERYBUF, &(fg->buffers[i].buffer)) == -1)
  {
   fprintf(stderr, "ioctl error (VIDIOC_QUERYBUF)\n");
   close(fg->dev_fd); fg->dev_fd = -1;
   free(fg->image);
   fg->image = NULL;
   if (fg->bayerbuf) free(fg->bayerbuf);
   fg->bayerbuf = NULL;
   fg->bayerbuf_size = 0;
   free(fg);
   return NULL;
  }
 }
 for (i = 0; i < (fg->buffers_num); i++)
 {
  fg->buffers[i].video_map = mmap(NULL, fg->buffers[i].buffer.length, PROT_READ|PROT_WRITE, MAP_SHARED, fg->dev_fd, fg->buffers[i].buffer.m.offset);
  if (fg->buffers[i].video_map == MAP_FAILED)
  {
   fprintf(stderr, "cannot mmap()\n");
   fg->buffers[i].video_map = NULL;
   for (i = 0; i < REQUEST_BUFFERS; i++)
   {
    if (fg->buffers[i].video_map)
    {
     munmap(fg->buffers[i].video_map, fg->buffers[i].buffer.length);
     fg->buffers[i].video_map = NULL;
    }
   }
   close(fg->dev_fd); fg->dev_fd = -1;
   free(fg->image);
   fg->image = NULL;
   if (fg->bayerbuf) free(fg->bayerbuf);
   fg->bayerbuf = NULL;
   fg->bayerbuf_size = 0;
   free(fg);
   return NULL;
  }
 }
 return fg;
}

void close_fg(void * fg)
{
 int i;

 if (FG(fg)->grabbing) stop_grab(fg);
 for (i = 0; i < REQUEST_BUFFERS; i++)
 {
  if (FG(fg)->buffers[i].video_map)
  {
   munmap(FG(fg)->buffers[i].video_map, FG(fg)->buffers[i].buffer.length);
   FG(fg)->buffers[i].video_map = NULL;
  }
 }
 if (FG(fg)->image) free(FG(fg)->image);
 FG(fg)->image = NULL;
 if (FG(fg)->bayerbuf) free(FG(fg)->bayerbuf);
 FG(fg)->bayerbuf = NULL;
 FG(fg)->bayerbuf_size = 0;
 close(FG(fg)->dev_fd); FG(fg)->dev_fd = -1;
 free(fg);
}
