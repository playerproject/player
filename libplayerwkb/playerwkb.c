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

#include <stddef.h> /* NULL, size_t typedef and some versions of GEOS CAPI need this */
#include <string.h>
#include <playerconfig.h> /* this also includes <stdint.h> if needed for types like uint8_t */
#include <libplayercommon/playercommon.h>

#include "playerwkb.h"

#ifdef HAVE_GEOS

#ifndef GEOS_VERSION_MAJOR
#include <geos_c.h>
#endif

#if (GEOS_VERSION_MAJOR < 3 || GEOS_VERSION_MINOR < 1)
#undef HAVE_GEOS
#endif

#endif

#ifdef HAVE_GEOS

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

/** Dummy function passed as a function pointer GEOS when it is initialised. GEOS uses this for logging. */
void player_wkb_geosprint(const char * format, ...)
{
  va_list ap;
  va_start(ap, format);
  fprintf(stderr, "GEOSError: ");
  vfprintf(stderr,format, ap);
  fflush(stderr);
  va_end(ap);
};

#else

#include <assert.h>

#define WKB_POINT 1
#define WKB_LINESTRING 2
#define WKB_POLYGON 3
#define WKB_MULTIPOINT 4
#define WKB_MULTILINESTRING 5
#define WKB_MULTIPOLYGON 6
#define WKB_GEOMETRYCOLLECTION 7

int player_wkb_endians_detect(struct PlayerWKBEndians * endians)
{
  int ui32_one = 1;
  double dbl_one = 1.0;
  uint8_t uint32_one_be[] = { 0, 0, 0, 1 };
  uint8_t uint32_one_le[] = { 1, 0, 0, 0 };
  uint8_t dbl_one_be[] = { 0x3f, 0xf0, 0, 0, 0, 0, 0, 0 };
  uint8_t dbl_one_le[] = { 0, 0, 0, 0, 0, 0, 0xf0, 0x3f };
  const uint8_t * bytes;

  memset(endians, 0, sizeof(struct PlayerWKBEndians));
  if (sizeof(double) != 8)
  {
    PLAYER_ERROR("incompatible size of double");
    return -1;
  }
  if (sizeof(uint32_t) != 4)
  {
    PLAYER_ERROR("incompatible size of uint32_t");
    return -1;
  }
  bytes = (uint8_t *)(&ui32_one);
  if (!memcmp(uint32_one_be, bytes, sizeof(uint32_t)))
  {
    endians->uint32_endians = player_wkb_big;
  } else if (!memcmp(uint32_one_le, bytes, sizeof(uint32_t)))
  {
    endians->uint32_endians = player_wkb_little;
  } else
  {
    PLAYER_ERROR("unknown integer endians");
    return -1;
  }
  bytes = (uint8_t *)(&dbl_one);
  if (!memcmp(dbl_one_be, bytes, sizeof(double)))
  {
    endians->dbl_endians = player_wkb_big;
  } else if (!memcmp(dbl_one_le, bytes, sizeof(double)))
  {
    endians->dbl_endians = player_wkb_little;
  } else
  {
    PLAYER_ERROR("unknown double precision endians");
    return -1;
  }
  return 0;
}

#endif

playerwkbprocessor_t player_wkb_create_processor()
{
#ifdef HAVE_GEOS
  return (playerwkbprocessor_t)(initGEOS_r(player_wkb_geosprint, player_wkb_geosprint));
#else
  return NULL;
#endif
}

void player_wkb_destroy_processor(playerwkbprocessor_t wkbprocessor)
{
#ifdef HAVE_GEOS
  finishGEOS_r((GEOSContextHandle_t)wkbprocessor);
#else
  wkbprocessor = wkbprocessor;
#endif
}

#ifdef HAVE_GEOS

void player_wkb_process_geom(playerwkbprocessor_t wkbprocessor, const GEOSGeometry * geom, playerwkbcallback_t callback, void * ptr)
{
  double x0, y0, x1, y1;
  const GEOSCoordSequence * seq;
  size_t numcoords;
  int i;

  switch (GEOSGeomTypeId_r((GEOSContextHandle_t)wkbprocessor, geom))
  {
  case GEOS_POINT:
    seq = GEOSGeom_getCoordSeq_r((GEOSContextHandle_t)wkbprocessor, geom);
    if (!seq)
    {
      PLAYER_ERROR("Invalid coordinate sequence");
      return;
    }
    GEOSCoordSeq_getX(seq, 0, &x0);
    GEOSCoordSeq_getY(seq, 0, &y0);
    callback(ptr, x0 - 0.1, y0, x0 + 0.1, y0);
    break;
  case GEOS_LINESTRING:
  case GEOS_LINEARRING:
    seq = GEOSGeom_getCoordSeq_r((GEOSContextHandle_t)wkbprocessor, geom);
    if (GEOSCoordSeq_getSize_r((GEOSContextHandle_t)wkbprocessor, seq, (unsigned int *)(&numcoords)))
    {
      if (numcoords > 0)
      {
        GEOSCoordSeq_getX_r((GEOSContextHandle_t)wkbprocessor, seq, 0, &x1);
	GEOSCoordSeq_getY_r((GEOSContextHandle_t)wkbprocessor, seq, 0, &y1);
        if (numcoords < 2)
        {
          callback(ptr, x1 - 0.1, y1, x1 + 0.1, y1);
          callback(ptr, x1, y1 - 0.1, x1, y1 + 0.1);
        } else for (i = 0; i < (signed)numcoords; i++)
        {
          x0 = x1;
          y0 = y1;
          GEOSCoordSeq_getX_r((GEOSContextHandle_t)wkbprocessor, seq, i, &x1);
          GEOSCoordSeq_getY_r((GEOSContextHandle_t)wkbprocessor, seq, i, &y1);
	  callback(ptr, x0, y0, x1, y1);
        }
      }
    }
    break;
  case GEOS_POLYGON:
    player_wkb_process_geom(wkbprocessor, GEOSGetExteriorRing_r((GEOSContextHandle_t)wkbprocessor, geom), callback, ptr);
    numcoords = GEOSGetNumInteriorRings_r((GEOSContextHandle_t)wkbprocessor, geom);
    for (i = 0; i < (signed)numcoords; i++) player_wkb_process_geom(wkbprocessor, GEOSGetInteriorRingN_r((GEOSContextHandle_t)wkbprocessor, geom, i), callback, ptr);
    break;
  case GEOS_MULTIPOINT:
  case GEOS_MULTILINESTRING:
  case GEOS_MULTIPOLYGON:
  case GEOS_GEOMETRYCOLLECTION:
    numcoords = GEOSGetNumGeometries_r((GEOSContextHandle_t)wkbprocessor, geom);
    for (i = 0; i < (signed)numcoords; i++) player_wkb_process_geom(wkbprocessor, GEOSGetGeometryN_r((GEOSContextHandle_t)wkbprocessor, geom, i), callback, ptr);
    break;
  default:
    PLAYER_WARN("unknown feature type!");
  }
}

#endif

const uint8_t * player_wkb_process_wkb(playerwkbprocessor_t wkbprocessor, const uint8_t * wkb, size_t wkb_count, playerwkbcallback_t callback, void * ptr)
{
#ifdef HAVE_GEOS

  GEOSGeometry * geom;

  if (!wkb)
  {
    PLAYER_ERROR("NULL wkb");
    return NULL;
  }
  geom = GEOSGeomFromWKB_buf_r((GEOSContextHandle_t)wkbprocessor, wkb, wkb_count);
  if (!geom)
  {
    PLAYER_ERROR("cannot read wkb");
    return NULL;
  }
  player_wkb_process_geom(wkbprocessor, geom, callback, ptr);
  GEOSGeom_destroy_r((GEOSContextHandle_t)wkbprocessor, geom);
  return wkb + wkb_count;

#else

  double x0, y0, x1, y1;
  struct PlayerWKBEndians endians;
  uint32_t numcoords, numrings;
  int i, j;
  uint32_t type;
  enum player_wkb_endians wkb_endians;

#define UINT_FROM_WKB(dst) do \
{ \
  assert(wkb_count >= 4); \
  if (wkb_endians == (endians.uint32_endians)) memcpy((dst), wkb, 4); \
  else \
  { \
    ((uint8_t *)(dst))[0] = wkb[(4 - 1) - 0]; \
    ((uint8_t *)(dst))[1] = wkb[(4 - 1) - 1]; \
    ((uint8_t *)(dst))[2] = wkb[(4 - 1) - 2]; \
    ((uint8_t *)(dst))[3] = wkb[(4 - 1) - 3]; \
  } \
  wkb += 4; wkb_count -= 4; \
} while (0)

#define DBL_FROM_WKB(dst) do \
{ \
  assert(wkb_count >= 8); \
  if (wkb_endians == (endians.dbl_endians)) memcpy((dst), wkb, 8); \
  else \
  { \
    ((uint8_t *)(dst))[0] = wkb[(8 - 1) - 0]; \
    ((uint8_t *)(dst))[1] = wkb[(8 - 1) - 1]; \
    ((uint8_t *)(dst))[2] = wkb[(8 - 1) - 2]; \
    ((uint8_t *)(dst))[3] = wkb[(8 - 1) - 3]; \
    ((uint8_t *)(dst))[4] = wkb[(8 - 1) - 4]; \
    ((uint8_t *)(dst))[5] = wkb[(8 - 1) - 5]; \
    ((uint8_t *)(dst))[6] = wkb[(8 - 1) - 6]; \
    ((uint8_t *)(dst))[7] = wkb[(8 - 1) - 7]; \
  } \
  wkb += 8; wkb_count -= 8; \
} while (0)

  if (!wkb)
  {
    PLAYER_ERROR("NULL wkb");
    return NULL;
  }
  if (player_wkb_endians_detect(&endians)) return NULL;
  assert(wkb_count > 1);
  wkb_endians = (enum player_wkb_endians)(wkb[0]); wkb++; wkb_count--;
  if ((wkb_endians != player_wkb_big) && (wkb_endians != player_wkb_little))
  {
    PLAYER_ERROR1("invalid wkb: unknown endians, %d", wkb_endians);
    return NULL;
  }
  UINT_FROM_WKB(&type);
  switch (type)
  {
  case WKB_POINT:
    DBL_FROM_WKB(&x0);
    DBL_FROM_WKB(&y0);
    callback(ptr, x0 - 0.1, y0, x0 + 0.1, y0);
    callback(ptr, x0, y0 - 0.1, x0, y0 + 0.1);
    break;
  case WKB_LINESTRING:
    UINT_FROM_WKB(&numcoords);
    if (numcoords > 0)
    {
      DBL_FROM_WKB(&x1);
      DBL_FROM_WKB(&y1);
      if (numcoords < 2)
      {
        callback(ptr, x1 - 0.1, y1, x1 + 0.1, y1);
        callback(ptr, x1, y1 - 0.1, x1, y1 + 0.1);
      } else for (i = 1; i < ((signed)(numcoords)); i++)
      {
	x0 = x1;
	y0 = y1;
	DBL_FROM_WKB(&x1);
	DBL_FROM_WKB(&y1);
	callback(ptr, x0, y0, x1, y1);
      }
    }
    break;
  case WKB_POLYGON:
    UINT_FROM_WKB(&numrings);
    for (i = 0; i < (signed)(numrings); i++)
    {
      UINT_FROM_WKB(&numcoords);
      if (numcoords > 0)
      {
	DBL_FROM_WKB(&x1);
	DBL_FROM_WKB(&y1);
	if (numcoords < 2)
	{
          callback(ptr, x1 - 0.1, y1, x1 + 0.1, y1);
	  callback(ptr, x1, y1 - 0.1, x1, y1 + 0.1);
	} else for (j = 1; j < (signed)(numcoords); j++)
	{
	  x0 = x1;
	  y0 = y1;
	  DBL_FROM_WKB(&x1);
	  DBL_FROM_WKB(&y1);
	  callback(ptr, x0, y0, x1, y1);
	}
      }
    }
    break;
  case WKB_MULTIPOINT:
  case WKB_MULTILINESTRING:
  case WKB_MULTIPOLYGON:
  case WKB_GEOMETRYCOLLECTION:
    UINT_FROM_WKB(&numrings);
    for (i = 0; i < (signed)(numrings); i++)
    {
      wkb = player_wkb_process_wkb(wkbprocessor, wkb, wkb_count, callback, ptr);
      if (!wkb) return NULL;
    }
    break;
  default:
    PLAYER_WARN("unknown wkb feature type!");
    return NULL;
  }
  return wkb;
#undef UINT_FROM_WKB
#undef DBL_FROM_WKB

#endif
}

size_t player_wkb_create_linestring(playerwkbprocessor_t wkbprocessor, double (* shape)[2], size_t shape_num_points, double offsetx, double offsety, uint8_t * dest_wkb, size_t max_size)
{
#ifdef HAVE_GEOS

  GEOSCoordSequence * seq;
  GEOSGeometry * geom;
  unsigned char * wkb;
  size_t s;
  int i;

  if (!shape)
  {
    PLAYER_ERROR("NULL shape");
    return 0;
  }
  if (!(shape_num_points > 1)) return 0;
  if (dest_wkb)
  {
    if (!(max_size > 0)) return 0;
  }
  seq = GEOSCoordSeq_create_r((GEOSContextHandle_t)wkbprocessor, shape_num_points, 2);
  if (!seq)
  {
    PLAYER_ERROR("cannot create geometry sequence");
    return 0;
  }
  for (i = 0; i < ((int)(shape_num_points)); i++)
  {
    GEOSCoordSeq_setX_r((GEOSContextHandle_t)wkbprocessor, seq, i, shape[i][0] + offsetx);
    GEOSCoordSeq_setY_r((GEOSContextHandle_t)wkbprocessor, seq, i, shape[i][1] + offsety);
  }  
  geom = GEOSGeom_createLineString_r((GEOSContextHandle_t)wkbprocessor, seq);
  if (!geom)
  {
    GEOSCoordSeq_destroy_r((GEOSContextHandle_t)wkbprocessor, seq);
    PLAYER_ERROR("cannot create linestring geometry");
    return 0;
  }
  s = 0;
  wkb = GEOSGeomToWKB_buf_r((GEOSContextHandle_t)wkbprocessor, geom, &s);
  if (!wkb)
  {
    GEOSGeom_destroy_r((GEOSContextHandle_t)wkbprocessor, geom);
    /* !!! do not call: GEOSCoordSeq_destroy_r((GEOSContextHandle_t)wkbprocessor, seq); */
    PLAYER_ERROR("cannot create linestring wkb");
    return 0;
  }
  if (dest_wkb)
  {
    if ((!(s > 0)) || (s > max_size))
    {
      free(wkb);
      GEOSGeom_destroy_r((GEOSContextHandle_t)wkbprocessor, geom);
      /* !!! do not call: GEOSCoordSeq_destroy_r((GEOSContextHandle_t)wkbprocessor, seq); */
      PLAYER_ERROR("invalid linestring wkb size");
      return 0;    
    }
    memcpy(dest_wkb, wkb, s);
  }
  free(wkb);
  GEOSGeom_destroy_r((GEOSContextHandle_t)wkbprocessor, geom);
  /* !!! do not call: GEOSCoordSeq_destroy_r((GEOSContextHandle_t)wkbprocessor, seq); */
  return s;

#else

  struct PlayerWKBEndians endians;
  size_t s = 0;
  int i;
  uint32_t ui;
  double dbl;

#define WKB_FROM_UINT(src) do \
{ \
  if (dest_wkb) \
  { \
    assert(max_size - s >= 4); \
    if (player_wkb_big == (endians.uint32_endians)) memcpy(dest_wkb, (src), 4); \
    else \
    { \
      dest_wkb[0] = ((uint8_t *)(src))[(4 - 1) - 0]; \
      dest_wkb[1] = ((uint8_t *)(src))[(4 - 1) - 1]; \
      dest_wkb[2] = ((uint8_t *)(src))[(4 - 1) - 2]; \
      dest_wkb[3] = ((uint8_t *)(src))[(4 - 1) - 3]; \
    } \
  } \
  s += 4; if (dest_wkb) dest_wkb += 4; \
} while (0)

#define WKB_FROM_DBL(src) do \
{ \
  if (dest_wkb) \
  { \
    assert(max_size - s >= 8); \
    if (player_wkb_big == (endians.dbl_endians)) memcpy(dest_wkb, (src), 8); \
    else \
    { \
      dest_wkb[0] = ((uint8_t *)(src))[(8 - 1) - 0]; \
      dest_wkb[1] = ((uint8_t *)(src))[(8 - 1) - 1]; \
      dest_wkb[2] = ((uint8_t *)(src))[(8 - 1) - 2]; \
      dest_wkb[3] = ((uint8_t *)(src))[(8 - 1) - 3]; \
      dest_wkb[4] = ((uint8_t *)(src))[(8 - 1) - 4]; \
      dest_wkb[5] = ((uint8_t *)(src))[(8 - 1) - 5]; \
      dest_wkb[6] = ((uint8_t *)(src))[(8 - 1) - 6]; \
      dest_wkb[7] = ((uint8_t *)(src))[(8 - 1) - 7]; \
    } \
  } \
  s += 8; if (dest_wkb) dest_wkb += 8; \
} while (0)

  wkbprocessor = wkbprocessor;
  if (!shape)
  {
    PLAYER_ERROR("NULL shape");
    return 0;
  }
  if (!(shape_num_points > 1)) return 0;
  if (dest_wkb)
  {
    if (!(max_size > 0)) return 0;
    if (player_wkb_endians_detect(&endians)) return 0;
    *dest_wkb = ((uint8_t)(player_wkb_big));
  }
  s++; if (dest_wkb) dest_wkb++;
  ui = WKB_LINESTRING;
  WKB_FROM_UINT(&ui);
  ui = ((uint32_t)(shape_num_points));
  WKB_FROM_UINT(&ui);
  for (i = 0; i < ((int)(shape_num_points)); i++)
  {
    dbl = shape[i][0] + offsetx;
    WKB_FROM_DBL(&dbl);
    dbl = shape[i][1] + offsety;
    WKB_FROM_DBL(&dbl);
  }
  return s;
#undef WKB_FROM_UINT
#undef WKB_FROM_DBL

#endif
}
