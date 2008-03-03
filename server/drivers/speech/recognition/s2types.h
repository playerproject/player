/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

/*
 * prim_type.h -- Primitive types; more machine-independent.
 *
 * **********************************************
 * CMU ARPA Speech Project
 *
 * Copyright (c) 1999 Carnegie Mellon University.
 * ALL RIGHTS RESERVED.
 * **********************************************
 * 
 * HISTORY
 * 
 * $Log$
 * Revision 1.2  2008/03/03 21:46:04  gerkey
 * inserted new s2types.h, following user experience
 *
 * Revision 1.1  2006/04/20 01:05:29  gerkey
 * changed sphinx2 configure check, added s2types.h
 *
 * Revision 1.7  2005/10/04 23:56:30  dhdfu
 * Explicitly define int8 as signed, otherwise it will be unsigned on ARM
 * and PowerPC, and Sphinx3 model definition files won't work.
 *
 * Revision 1.6  2004/12/10 16:48:56  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 12-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added some useful constant definitions.
 * 
 * Revision 1.5  2004/07/16 00:57:10  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.1.1.1  2004/03/01 14:30:20  rkm
 *
 *
 * Revision 1.3  2004/02/09 21:19:35  rkm
 * Added bool
 *
 * Revision 1.2  2004/01/23 19:20:03  rkm
 * *** empty log message ***
 *
 *
 * 02-Aug-1999  Kevin A. Lenzo (lenzo@cs.cmu.edu) at Carnegie Mellon
 *              Copied from s3 and cut out everything but the typedefs.
 * 
 * 12-Mar-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added arraysize_t, point_t, fpoint_t.
 * 
 * 01-Feb-1999	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Added anytype_t.
 * 
 * 08-31-95	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon
 * 		Created.
 */


#ifndef __S2TYPES_H__
#define __S2TYPES_H__


typedef int		int32;
typedef short		int16;
typedef signed char	int8;
typedef unsigned int	uint32;
typedef unsigned short	uint16;
typedef unsigned char	uint8;
typedef float		float32;
typedef double		float64;
typedef unsigned char	boolean;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL (void *)0
#endif


/* Useful constants */
#define MAX_INT32		((int32) 0x7fffffff)
#define MAX_INT16		((int16) 0x00007fff)
#define MAX_INT8		((int8)  0x0000007f)

#define MAX_NEG_INT32		((int32) 0x80000000)
#define MAX_NEG_INT16		((int16) 0xffff8000)
#define MAX_NEG_INT8		((int8)  0xffffff80)

#define MAX_UINT64		((uint64) 0xffffffffffffffff)
#define MAX_UINT32		((uint32) 0xffffffff)
#define MAX_UINT16		((uint16) 0x0000ffff)
#define MAX_UINT8		((uint8)  0x000000ff)

/* The following are approximate; IEEE floating point standards might quibble! */
#define MAX_POS_FLOAT32		3.4e+38f
#define MIN_POS_FLOAT32		1.2e-38f	/* But not 0 */
#define MAX_POS_FLOAT64		1.8e+307
#define MIN_POS_FLOAT64		2.2e-308

/* Will the following really work?? */
#define MAX_NEG_FLOAT32		((float32) (-MAX_POS_FLOAT32))
#define MIN_NEG_FLOAT32		((float32) (-MIN_POS_FLOAT32))
#define MAX_NEG_FLOAT64		((float64) (-MAX_POS_FLOAT64))
#define MIN_NEG_FLOAT64		((float64) (-MIN_POS_FLOAT64))

/*
typedef union anytype_s {
  boolean boolean;
  int8 int8;
  uint8 uint8;
  int16 int16;
  uint16 uint16;
  int32 int32;
  uint32 uint32;
  float32 float32;
  float64 float64;
  void *ptr;
} anytype_t;
*/

#endif
