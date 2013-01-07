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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: AMD Geode SoC camera support for camerav4l2 driver
// Author: Paul Osmialowski
//
///////////////////////////////////////////////////////////////////////////

#include "geode.h"
#include "v4l2.h"
#include <unistd.h>
#include <sys/types.h>
#include <linux/videodev2.h>
#include <config.h>
#if HAVE_I2C
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#ifndef V4L2_CID_CAM_INIT
#define V4L2_CID_CAM_INIT               (V4L2_CID_BASE+33)
#endif

int geode_select_cam(const char * dev, int cam)
{
#if HAVE_I2C
  int i;
  struct i2c_smbus_ioctl_data args;
  union i2c_smbus_data data;

  memset(&args, 0, sizeof args);
  memset(&data, 0, sizeof data);
  if (!((cam == 1) || (cam == 2))) return -1;
  i = open(dev, O_RDWR);
  if (i == -1) return -1;
  if (ioctl(i, 0x703, 8) == -1)
  {
    close(i);
    return -1;
  }
  args.read_write = I2C_SMBUS_READ;
  args.command = 170;
  args.size = I2C_SMBUS_BYTE_DATA;
  args.data = &data;
  if (ioctl(i, I2C_SMBUS, &args) == -1)
  {
    close(i);
    return -1;
  }
  args.read_write = I2C_SMBUS_WRITE;
  args.command = 220;
  args.size = I2C_SMBUS_BLOCK_DATA;
  args.data = &data;
  data.block[0] = 1;
  data.block[1] = cam;
  if (ioctl(i, I2C_SMBUS, &args) == -1)
  {
    close(i);
    return -1;
  }
  close(i);
  return 0;
#else
#warning I2C kernel headers cannot be used, geode_select_cam() function will always fail
  dev = dev; cam = cam;
  return -1;
#endif
}

void * geode_open_fg(const char * dev, const char * pixformat, int width, int height, int imgdepth, int buffers)
{
 int i;
 struct v4l2_requestbuffers reqbuf;
 struct v4l2_format format;
 struct v4l2_control control;
 struct v4l2_streamparm fps;
 struct fg_struct * fg;
 v4l2_std_id esid0;

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
 switch (width)
 {
 case 320:
  if (height != 240)
  {
   fprintf(stderr, "invalid image size\n");
   free(fg);
   return NULL;
  }
  break;
 case 640:
  if (height != 480)
  {
   fprintf(stderr, "invalid image size\n");
   free(fg);
   return NULL;
  }
  break;
 default:
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
 if ((fg->pixformat) == v4l2_fmtbyname("YUYV"))
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
 memset(&control, 0, sizeof control);
 control.id = V4L2_CID_CAM_INIT;
 control.value = 0;
 if (ioctl(fg->dev_fd, VIDIOC_S_CTRL, &control) < 0)
 {
  fprintf(stderr, "ioctl error (V4L2_CID_CAM_INIT)\n");
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 esid0 = (width == 320) ? 0x04000000UL : 0x08000000UL;
 if (ioctl(fg->dev_fd, VIDIOC_S_STD, &esid0))
 {
  fprintf(stderr, "ioctl error (VIDIOC_S_STD)\n");
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
 format.fmt.pix.field = V4L2_FIELD_NONE;
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
 if (format.fmt.pix.sizeimage != ((fg->pixels) * (fg->depth)))
 {
  fprintf(stderr, "unexpected size change\n");
  close(fg->dev_fd); fg->dev_fd = -1;
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 memset(&fps, 0, sizeof fps);
 fps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
 if (ioctl(fg->dev_fd, VIDIOC_G_PARM, &fps))
 {
  fprintf(stderr, "ioctl error (VIDIOC_G_PARM)\n");
  close(fg->dev_fd); fg->dev_fd = -1;
  free(fg->image);
  fg->image = NULL;
  if (fg->bayerbuf) free(fg->bayerbuf);
  fg->bayerbuf = NULL;
  fg->bayerbuf_size = 0;
  free(fg);
  return NULL;
 }
 fps.parm.capture.timeperframe.numerator = 1;
 fps.parm.capture.timeperframe.denominator = 30;
 if (ioctl(fg->dev_fd, VIDIOC_S_PARM, &fps) == -1)
 {
  fprintf(stderr, "ioctl error (VIDIOC_S_PARM)\n");
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
  fg->buffers[i].video_map = mmap(NULL, fg->buffers[i].buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fg->dev_fd, fg->buffers[i].buffer.m.offset);
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
