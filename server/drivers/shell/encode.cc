
/*
 * Desc: Useful encoding/decoding routines
 * Author: Andrew Howard
 * Date: 16 Sep 2005
 * CVS: $Id$
 */

#include <stdlib.h>
#include <string.h>

#include "encode.h"


static char hex_table[] = "0123456789ABCDEF";


////////////////////////////////////////////////////////////////////////////
// Encode binary data to ascii hex
void EncodeHex(const void *src, int len, char **dst)
{
  int i;
  int s, sl, sh;
  
  // Create a buffer for the data
  *dst = (char*) calloc(2 * len + 1, 1);

  for (i = 0; i < len; i++)
  {
    s = ((const unsigned char*) src)[i];
    sl = s & 0x0F;
    sh = (s >> 4) & 0x0F;
    (*dst)[i * 2 + 0] = hex_table[sh];
    (*dst)[i * 2 + 1] = hex_table[sl];
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////
// Decodes ascii hex to binary data.  
void DecodeHex(const char *src, void **dst, int *len)
{
  int i;
  int sl, sh;

  *len = strlen(src) / 2;
  *dst = calloc(*len, 1);

  for (i = 0; i < *len; i++)
  {
    sh = src[2 * i + 0];
    sl = src[2 * i + 1];
    ((unsigned char*) *dst) = (sh << 4) | sl;
  }
  
  return;
}

