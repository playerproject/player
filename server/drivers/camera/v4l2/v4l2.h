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
// Desc: Video4Linux2 routines for camerav4l2 driver
// Author: Paul Osmialowski
//
///////////////////////////////////////////////////////////////////////////

#ifndef _V4L2_H
#define _V4L2_H

#include <sys/types.h>
#include <linux/videodev2.h>

#ifdef __cplusplus
extern "C" {
#endif

#define v4l2_fmtbyname(name) v4l2_fourcc((name)[0], (name)[1], (name)[2], (name)[3])

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

extern void * open_fg(const char * dev, const char * pixformat, int width, int height, int imgdepth, int buffers);
extern void close_fg(void * fg);
extern int set_channel(void * fg, int channel, const char * mode);
extern int start_grab (void * fg);
extern void stop_grab (void * fg);
extern unsigned char * get_image(void * fg);
extern int fg_width(void * fg);
extern int fg_height(void * fg);
extern int fg_grabdepth(void * fg);
extern int fg_imgdepth(void * fg);

#ifdef __cplusplus
}
#endif

#endif
