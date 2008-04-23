#include <config.h>

/* make sure we get the various types like 'uint8_t
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

#include <sys/types.h>
#if HAVE_STDINT_H
  #include <stdint.h>
#endif

/*
 * 64-bit conversion macros
 */
#if WORDS_BIGENDIAN
  #define htonll(n) (n)
#else
  #define htonll(n) ((((unsigned long long) htonl(n)) << 32) + htonl((n) >> 32))
#endif

#if WORDS_BIGENDIAN
  #define ntohll(n) (n)
#else
  #define ntohll(n) ((((unsigned long long)ntohl(n)) << 32) + ntohl((n) >> 32))
#endif
