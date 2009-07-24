/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2009
 *     Paul Osmialowski
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __PLAYER_WKB_H
#define __PLAYER_WKB_H

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERWKB_EXPORT
  #elif defined (playerwkb_EXPORTS)
    #define PLAYERWKB_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERWKB_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERWKB_EXPORT
#endif

#include <stddef.h> /* size_t typedef and some versions of GEOS CAPI need this */
#include <playerconfig.h> /* this also includes <stdint.h> if needed for types like uint8_t */

#ifdef HAVE_GEOS

#ifndef GEOS_VERSION_MAJOR
#include <geos_c.h>
#endif

#if (GEOS_VERSION_MAJOR < 3 || GEOS_VERSION_MINOR < 1)
#undef HAVE_GEOS
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_GEOS

PLAYERWKB_EXPORT typedef GEOSContextHandle_t playerwkbprocessor_t;

#else

PLAYERWKB_EXPORT typedef void * playerwkbprocessor_t;

PLAYERWKB_EXPORT enum player_wkb_endians { player_wkb_big = 0, player_wkb_little = 1 };

struct PlayerWKBEndians
{
  enum player_wkb_endians uint32_endians, dbl_endians;
};

#endif

PLAYERWKB_EXPORT typedef void (* playerwkbcallback_t)(void *, double, double, double, double);

PLAYERWKB_EXPORT playerwkbprocessor_t player_wkb_create_processor();
PLAYERWKB_EXPORT void player_wkb_destroy_processor(playerwkbprocessor_t wkbprocessor);
PLAYERWKB_EXPORT const uint8_t * player_wkb_process_wkb(playerwkbprocessor_t wkbprocessor, const uint8_t * wkb, size_t wkb_count, playerwkbcallback_t callback, void * ptr);
PLAYERWKB_EXPORT size_t player_wkb_create_linestring(playerwkbprocessor_t wkbprocessor, double (* shape)[2], size_t shape_num_points, double offsetx, double offsety, uint8_t * dest_wkb, size_t max_size);

#ifdef __cplusplus
}
#endif

#endif
