
/*
 * Desc: Useful encoding/decoding routines
 * Author: Andrew Howard
 * Date: 16 Sep 2005
 * CVS: $Id$
 */

#ifndef ENCODE_H_
#define ENCODE_H_


/// Encode binary data to ascii hex.  The caller is reponsible for
/// freeing the destination string.
void EncodeHex(const void *src, int len, char **dst);

/// Decodes ascii hex to binary data.  The caller is reponsible for
/// freeing the destination buffer.
void DecodeHex(const char *src, char **dst, int *len);


#endif
