/* $Author$
 * $Name$
 * $Id$
 * $Source$
 * $Log$
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
#include <jpeglib.h>
#include <jerror.h>
#include <setjmp.h>
struct my_error_mgr {
	struct jpeg_error_mgr pub;
	jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;


int 
jpeg_compress(char *dst, char *src, int width, int height, int dstsize, int quality);

void
jpeg_decompress(unsigned char *dst, unsigned char *src, int size, int *width, int *height);

void
jpeg_decompress_from_file(unsigned char *dst, char *file, int size, int *width, int *height);

#ifdef __cplusplus
}
#endif

#endif
