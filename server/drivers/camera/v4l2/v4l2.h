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

#ifndef _V4L2_H
#define _V4L2_H

#ifdef __cplusplus
extern "C" {
#endif

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
