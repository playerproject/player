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
///////////////////////////////////////////////////////////////////////////
//
// Desc: jpeg compress routines
// Author: Nate Koenig 
// Date: 20 Aug 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

#ifndef JPEGCOMPRESS_H
#define JPEGCOMPRESS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <jpeglib.h>
#include <player.h>

// Converts rawImage into a jpeg image that is stored within a 
// player_camera_data structure. Quality should range from 0-100 with
// 0 being the worst quality (most compression).
void jpeg_compress( unsigned char *rawImage, player_camera_data_t *data, 
                    int quality );



// 
// !! DO NOT directly call these routine !!
//
typedef struct {
  struct jpeg_destination_mgr pub;

  unsigned int bufferSize;

  JOCTET *buffer; /*start of buffer*/
} MyDestinationMgr;

typedef MyDestinationMgr* MyDestPtr;

void init_destination( j_compress_ptr cinfo );

boolean empty_output_buffer( j_compress_ptr cinfo );

void term_destination( j_compress_ptr cinfo );

void jpeg_mem_dest( j_compress_ptr cinfo );


#ifdef __cplusplus
}
#endif

#endif
