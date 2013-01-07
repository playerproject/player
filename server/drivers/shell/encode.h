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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/*
 * Desc: Useful encoding/decoding routines
 * Author: Andrew Howard
 * Date: 16 Sep 2005
 * CVS: $Id$
 */

#ifndef ENCODE_H_
#define ENCODE_H_


/// Determine the size of the destination buffer for hex encoding
size_t EncodeHexSize(size_t src_len);

/// Encode binary data to ascii hex.
void EncodeHex(char *dst, size_t dst_len, const void *src, size_t src_len);

/// Determine the size of the destination buffer for hex decoding
size_t DecodeHexSize(size_t src_len);

/// Decodes ascii hex to binary data.  
void DecodeHex(void *dst, size_t dst_len, const char *src, size_t src_len);


#endif
