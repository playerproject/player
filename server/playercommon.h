/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *                      
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

/*
 * $Id$
 * 
 * common standard types and some generic size stuff.
 * also, debugging stuff.
 */

#ifndef _PLAYERCOMMON_H
#define _PLAYERCOMMON_H

#if HAVE_CONFIG_H
  #include <config.h>
#endif

/* make sure we get the various types like 'uint8_t' */
#if HAVE_SYS_TYPES_H
  #include <sys/types.h>
#endif
#if HAVE_STDINT_H
  #include <stdint.h>
#endif

//cygwin doesn't appear to have them 
//typedef unsigned int size_t; 
//typedef unsigned int uint32_t;
//typedef unsigned short uint16_t;
//typedef unsigned char uint8_t;
//typedef int int32_t;
//typedef short int16_t;
//typedef char int8_t;

// INT_MAX is more portable - limits.h
//#define MAXINT INT_MAX 

/* debug malloc(3) */
/* this should *not* be defined by default, because, as the man page says,
 * it causes "a special (less  efficient) implementation" of malloc() to be
 * used - BPG
 */
//#define MALLOC_CHECK_ 2

/* now, then.  we'll all use the following (ISO-endorsed) types:
 *
 * int8_t  : signed 1 byte  (char)
 * int16_t : signed 2 bytes (short)
 * int32_t : signed 4 bytes (int)
 * int64_t : signed 8 bytes (long)
 * 
 * uint8_t  : unsigned 1 byte  (unsigned char)
 * uint16_t : unsigned 2 bytes (unsigned short)
 * uint32_t : unsigned 4 bytes (unsigned int)
 * uint64_t : unsigned 8 bytes (unsigned long)
 */

/* 
 * there's no system-standard method for byte-swapping 64-bit quantities
 * (at least none that i could find), so here's a little macro
 */
/* but only if we need to swap */
#ifdef PLAYER_SOLARIS
  #define htonll(x) x
  #define ntohll(x) x
#else
  /* supporting bit masks */
  #define PLAYER_MASK_1in8   0xff00000000000000ULL
  #define PLAYER_MASK_2in8   0x00ff000000000000ULL
  #define PLAYER_MASK_3in8   0x0000ff0000000000ULL
  #define PLAYER_MASK_4in8   0x000000ff00000000ULL
  #define PLAYER_MASK_5in8   0x00000000ff000000ULL
  #define PLAYER_MASK_6in8   0x0000000000ff0000ULL
  #define PLAYER_MASK_7in8   0x000000000000ff00ULL
  #define PLAYER_MASK_8in8   0x00000000000000ffULL
  
  #define htonll(x)  ((uint64_t)\
           (((x & PLAYER_MASK_8in8) << 56) | ((x & PLAYER_MASK_7in8) << 40) | \
            ((x & PLAYER_MASK_6in8) << 24) | ((x & PLAYER_MASK_5in8) <<  8) | \
            ((x & PLAYER_MASK_4in8) >>  8) | ((x & PLAYER_MASK_3in8) >> 24) | \
            ((x & PLAYER_MASK_2in8) >> 40) | ((x & PLAYER_MASK_1in8) >> 56)))
  #define ntohll(x) htonll(x)
#endif


#define MAX_FILENAME_SIZE 256 // space for a relatively long pathname

////////////////////////////////////////////////////////////////////////////////
// Maths stuff

#ifndef M_PI
	#define M_PI        3.14159265358979323846
#endif

// Convert radians to degrees
//
#define RTOD(r) ((r) * 180 / M_PI)

// Convert degrees to radians
//
#define DTOR(d) ((d) * M_PI / 180)

// Normalize angle to domain -pi, pi
//
#define NORMALIZE(z) atan2(sin(z), cos(z))

#define LOBYTE(w) ((uint8_t) (w & 0xFF))
#define HIBYTE(w) ((uint8_t) ((w >> 8) & 0xFF))
#define MAKEUINT16(lo, hi) ((((uint16_t) (hi)) << 8) | ((uint16_t) (lo)))

#ifndef BOOL
	#define BOOL int
#endif

#ifndef TRUE
	#define TRUE true
#endif

#ifndef FALSE
	#define FALSE false
#endif

////////////////////////////////////////////////////////////////////////////////
// Array checking macros

// Macro for returning array size
//
#ifndef ARRAYSIZE
	// Note that the cast to int is used to suppress warning about
	// signed/unsigned mismatches.
	#define ARRAYSIZE(x) (int) (sizeof(x) / sizeof(x[0]))
#endif

// Macro for checking array bounds
//
#define ASSERT_INDEX(index, array) \
    ASSERT((size_t) (index) >= 0 && (size_t) (index) < sizeof(array) / sizeof(array[0]));


////////////////////////////////////////////////////////////////////////////////
// Misc useful stuff

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


////////////////////////////////////////////////////////////////////////////////
// Error, msg, trace macros

#include <assert.h>

#define ASSERT(m) assert(m)
#define VERIFY(m) assert(m)

#include <stdio.h>

/* too noisy! */
#define PLAYER_ERROR(m)         printf("player error : %s:%s():\n    "m"\n", __FILE__, __FUNCTION__)
#define PLAYER_ERROR1(m, a)     printf("player error : %s:%s():\n    "m"\n", __FILE__, __FUNCTION__, a)
#define PLAYER_ERROR2(m, a, b)  printf("player error : %s:%s():\n    "m"\n", __FILE__, __FUNCTION__, a, b)
//#define PLAYER_ERROR(m) 

// Warning macros
#define PLAYER_WARN(m)         printf("\nplayer warning : %s:%s():\n    "m"\n",\
                                     __FILE__, __FUNCTION__)
#define PLAYER_WARN1(m, a)     printf("\nplayer warning : %s:%s():\n    "m"\n",\
                                     __FILE__, __FUNCTION__, a)
#define PLAYER_WARN2(m, a, b)  printf("\nplayer warning : %s:%s():\n    "m"\n",\
                                     __FILE__, __FUNCTION__, a, b)
#define PLAYER_WARN3(m, a, b, c) printf("\nplayer warning : %s:%s():\n    "m"\n",\
                                     __FILE__, __FUNCTION__, a, b, c)


#if PLAYER_ENABLE_MSG

#define PLAYER_MSG0(m)             printf("player msg   : %s:%s():\n    "m"\n", \
                                          __FILE__, __FUNCTION__)   
#define PLAYER_MSG1(m, a)          printf("player msg   : %s:%s():\n    "m"\n", \
                                          __FILE__, __FUNCTION__, a)
#define PLAYER_MSG2(m, a, b)       printf("player msg   : %s:%s():\n    "m"\n", \
                                          __FILE__, __FUNCTION__, a, b)
#define PLAYER_MSG3(m, a, b, c)    printf("player msg   : %s:%s():\n    "m"\n", \
                                          __FILE__, __FUNCTION__, a, b, c) 
#define PLAYER_MSG4(m, a, b, c, d) printf("player msg   : %s:%s():\n    "m"\n", \
                                          __FILE__, __FUNCTION__, a, b, c, d)
#else

#define PLAYER_MSG0(m)
#define PLAYER_MSG1(m, a)
#define PLAYER_MSG2(m, a, b)
#define PLAYER_MSG3(m, a, b, c)
#define PLAYER_MSG4(m, a, b, c, d)

#endif

#if PLAYER_ENABLE_TRACE

#define PLAYER_TRACE0(m)             printf("debug : %s %s\n        "m"\n", \
                                            __FILE__, __FUNCTION__) 
#define PLAYER_TRACE1(m, a)          printf("debug : %s %s\n        "m"\n", \
                                            __FILE__, __FUNCTION__, a)
#define PLAYER_TRACE2(m, a, b)       printf("debug : %s %s\n        "m"\n", \
                                            __FILE__, __FUNCTION__, a, b)
#define PLAYER_TRACE3(m, a, b, c)    printf("debug : %s %s\n        "m"\n", \
                                            __FILE__, __FUNCTION__, a, b, c)
#define PLAYER_TRACE4(m, a, b, c, d) printf("debug : %s %s\n        "m"\n", \
                                            __FILE__, __FUNCTION__, a, b, c, d)

#else

#define PLAYER_TRACE0(m)
#define PLAYER_TRACE1(m, a)
#define PLAYER_TRACE2(m, a, b)
#define PLAYER_TRACE3(m, a, b, c)
#define PLAYER_TRACE4(m, a, b, c, d)

#endif



#endif
