#ifndef __PLAYER_WKB_H
#define __PLAYER_WKB_H

#include <playerconfig.h> /* this also includes <stdint.h> if needed for types like uint8_t */

#ifdef HAVE_GEOS

#ifndef GEOS_VERSION_MAJOR
#include <geos_c.h>
#endif

#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_GEOS

typedef GEOSContextHandle_t playerwkbprocessor_t;

#else

typedef void * playerwkbprocessor_t;

enum player_wkb_endians { player_wkb_big = 0, player_wkb_little = 1 };

struct PlayerWKBEndians
{
  enum player_wkb_endians uint32_endians, dbl_endians;
};

#endif

playerwkbprocessor_t player_wkb_create_processor();
void player_wkb_destroy_processor(playerwkbprocessor_t wkbprocessor);
const uint8_t * player_wkb_process_wkb(playerwkbprocessor_t wkbprocessor, const uint8_t * wkb, size_t wkb_count, void (* callback)(void *, double, double, double, double), void * ptr);

#ifdef __cplusplus
}
#endif

#endif
