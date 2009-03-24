#ifndef PLAYER_GEOS_H_
#define PLAYER_GEOS_H_

#ifndef GEOS_VERSION_MAJOR
#include <stddef.h>
#include <geos_c.h>
#endif

// workaround gcc 4.2's confusion over typedeffed constants
#if GEOS_VERSION_MAJOR >= 3
typedef const struct GEOSCoordSeq_t * const_GEOSCoordSeq;
typedef const struct GEOSGeom_t * const_GEOSGeom;
#else
typedef struct GEOSCoordSeq_t * const_GEOSCoordSeq;
typedef struct GEOSGeom_t * const_GEOSGeom;
#endif

#endif /* PLAYER_GEOS_H_ */
