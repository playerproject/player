/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Andrew Howard
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

/*
 * Desc: Useful encoding/decoding routines
 * Author: Andrew Howard
 * Date: 16 Sep 2005
 * CVS: $Id$
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "encode.h"


static char hex_table[] = "0123456789ABCDEF";
#define unhex_table(h) ((h >= 'A' && h <= 'F') ? 10 + h - 'A' : h - '0')


////////////////////////////////////////////////////////////////////////////
/// Determine the size of the destination buffer for hex encoding
size_t EncodeHexSize(size_t src_len)
{
  return src_len * 2;
}


////////////////////////////////////////////////////////////////////////////
// Encode binary data to ascii hex
void EncodeHex(char *dst, size_t dst_len, const void *src, size_t src_len)
{
  size_t i;
  int s, sl, sh;

  assert(dst_len >= 2 * src_len);
  
  for (i = 0; i < src_len; i++)
  {
    s = ((const unsigned char*) src)[i];
    sl = s & 0x0F;
    sh = (s >> 4) & 0x0F;
    dst[i * 2 + 0] = hex_table[sh];
    dst[i * 2 + 1] = hex_table[sl];
  }
  return;
}


////////////////////////////////////////////////////////////////////////////
/// Determine the size of the destination buffer for hex decoding
size_t DecodeHexSize(size_t src_len)
{
  return src_len / 2;
}


////////////////////////////////////////////////////////////////////////////
// Decodes ascii hex to binary data.  
void DecodeHex(void *dst, size_t dst_len, const char *src, size_t src_len)
{
  size_t i;
  int sl, sh;

  assert(dst_len >= src_len / 2);

  for (i = 0; i < dst_len; i++)
  {
    sh = unhex_table(src[2 * i + 0]);
    sl = unhex_table(src[2 * i + 1]);
    ((unsigned char*) dst)[i] = (sh << 4) | sl;
  }
  
  return;
}

