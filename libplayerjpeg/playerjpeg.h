/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005
 *     Brian Gerkey, Andrew Howard
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/* $Author$
 * $Name$
 * $Id$
 * $Source$
 * $Log$
 * Revision 1.1  2005/08/31 17:43:51  gerkey
 * created libplayerjpeg
 *
 * Revision 1.2  2004/11/22 23:10:17  gerkey
 * made libjpeg optional in libplayerpacket
 *
 * Revision 1.1  2004/09/17 18:09:05  inspectorg
 * *** empty log message ***
 *
 * Revision 1.1  2004/09/10 05:34:14  natepak
 * Added a JpegStream driver
 *
 * Revision 1.2  2003/12/30 20:49:49  srik
 * Added ifdef flags for compatibility
 *
 * Revision 1.1.1.1  2003/12/30 20:39:19  srik
 * Helicopter deployment with firewire camera
 *
 */

#ifndef _JPEG_H_
#define _JPEG_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

int 
jpeg_compress(char *dst, char *src, int width, int height, int dstsize, int quality);

void
jpeg_decompress(unsigned char *dst, int dst_size, unsigned char *src, int src_size);

void
jpeg_decompress_from_file(unsigned char *dst, char *file, int size, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif
