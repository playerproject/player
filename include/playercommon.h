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

/* make sure we get the various types like 'uint8_t' */
#ifdef PLAYER_SOLARIS
  #include <sys/types.h>   // solaris puts them here
#else
  #include <stdint.h>      // linux puts them here
#endif

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

//#define TRACE printf
#define TRACE(s) 
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))


////////////////////////////////////////////////////////////////////////////////
// Error, msg, trace macros

#include <assert.h>

#define ASSERT(m) assert(m)
#define VERIFY(m) assert(m)

#include <stdio.h>

/* too noisy! */
//#define ERROR(m)  printf("Error : %s : %s\n", __PRETTY_FUNCTION__, m)
#define ERROR(m) 
//#define MSG(m)       printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__)
#define MSG(m) 
//#define MSG1(m, a)   printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a)
#define MSG1(m, a) 
//#define MSG2(m, a, b) printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a, b)
#define MSG2(m, a, b) 
//#define MSG3(m, a, b, c) printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c)
#define MSG3(m, a, b, c) 
//#define MSG4(m, a, b, c, d) printf("Msg   : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c, d)
#define MSG4(m, a, b, c, d) 

#if ENABLE_TRACE
    #define TRACE0(m)    printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__)
    #define TRACE1(m, a) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a)
    #define TRACE2(m, a, b) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a, b)
    #define TRACE3(m, a, b, c) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c)
    #define TRACE4(m, a, b, c, d) printf("Debug : %s : "m"\n", __PRETTY_FUNCTION__, a, b, c, d)
#else
    #define TRACE0(m)
    #define TRACE1(m, a)
    #define TRACE2(m, a, b)
    #define TRACE3(m, a, b, c)
    #define TRACE4(m, a, b, c, d)
#endif


#endif
