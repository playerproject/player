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

#include <string.h>

#include "jpegcompress.h"

#define OUTPUT_BUF_SIZE PLAYER_CAMERA_IMAGE_SIZE


// Initialize detination, called by jpeg_start_compress 
// before any data is actually written
void init_destination( j_compress_ptr cinfo )
{
  MyDestPtr dest = (MyDestPtr) cinfo->dest;

  // Allocate the output buffer, it will be released when done with image
  dest->buffer = (JOCTET *)(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo,
      JPOOL_IMAGE, OUTPUT_BUF_SIZE* sizeof(JOCTET));

  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;
}

// Empty the output buffer, called whenever the buffer fills up.
boolean empty_output_buffer( j_compress_ptr cinfo )
{
  MyDestPtr dest = (MyDestPtr) cinfo->dest;

  dest->bufferSize = OUTPUT_BUF_SIZE;


  dest->pub.next_output_byte = dest->buffer;
  dest->pub.free_in_buffer = OUTPUT_BUF_SIZE;

  return TRUE;
}

// Terminate Destination, calleb by jpeg_finish_compress
// After all data has been written. Usually needs to flush buffer.
void term_destination( j_compress_ptr cinfo )
{
  MyDestPtr dest = (MyDestPtr) cinfo->dest;
  size_t datacount = OUTPUT_BUF_SIZE - dest->pub.free_in_buffer;


  if (datacount > 0)
  {
    //printf("Left over buffer bytes [%d]\n",datacount);
    dest->bufferSize = datacount;
  }
}

// Prepare for output to a memory block.
//
void jpeg_mem_dest( j_compress_ptr cinfo )
{
  MyDestPtr dest;

  if (cinfo->dest == NULL)
  {
    cinfo->dest = cinfo->mem->alloc_small(
          (j_common_ptr) cinfo, JPOOL_PERMANENT, sizeof(MyDestinationMgr) );
  }

  dest = (MyDestPtr) cinfo->dest;
  dest->pub.init_destination = init_destination;
  dest->pub.empty_output_buffer = empty_output_buffer;
  dest->pub.term_destination = term_destination;
  dest->bufferSize = 0;
}


void jpeg_compress(unsigned char *rawImage, player_camera_data_t *data, int quality )
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  JSAMPROW rowPointer[1];
  int rowStride;

  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);

  jpeg_mem_dest(&cinfo);

  cinfo.image_width = ntohs(data->width);
  cinfo.image_height = ntohs(data->height);
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;

  jpeg_set_defaults(&cinfo);

  jpeg_set_quality(&cinfo, quality, TRUE);
  jpeg_start_compress(&cinfo, TRUE);

  rowStride = cinfo.image_width * 3;

  while(cinfo.next_scanline < cinfo.image_height)
  {
    rowPointer[0] = &rawImage[cinfo.next_scanline * rowStride];
    (void)jpeg_write_scanlines(&cinfo, rowPointer, 1);
  }

  jpeg_finish_compress(&cinfo);

  data->image_size = ((MyDestPtr)cinfo.dest)->bufferSize;
  memcpy(data->image,((MyDestPtr) cinfo.dest)->buffer, data->image_size);

  data->image_size = htonl(data->image_size);
  jpeg_destroy_compress(&cinfo);
}

