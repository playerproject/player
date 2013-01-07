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

#ifndef _GEODE_H
#define _GEODE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int geode_select_cam(const char * dev, int cam);
extern void * geode_open_fg(const char * dev, const char * pixformat, int width, int height, int imgdepth, int buffers);

#ifdef __cplusplus
}
#endif

#endif
