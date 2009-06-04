SET (playerwkbSrcs playerwkb.c)
PLAYER_ADD_LIBRARY (playerwkb ${playerwkbSrcs})
PLAYER_INSTALL_HEADERS (playerwkb playerwkb.h)

IF (HAVE_GEOS)
    TARGET_LINK_LIBRARIES (playerwkb geos_c)
    PLAYERCORE_ADD_INT_LINK_LIB (geos_c)
ENDIF (HAVE_GEOS)
