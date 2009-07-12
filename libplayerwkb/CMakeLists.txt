# Possibly use our own copy of XDR
#IF (NOT HAVE_XDR)
#INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
#ENDIF (NOT HAVE_XDR)

SET (playerwkbSrcs playerwkb.c)
PLAYER_ADD_LIBRARY (playerwkb ${playerwkbSrcs})

IF (HAVE_GEOS)
    TARGET_LINK_LIBRARIES (playerwkb playererror geos geos_c)
    PLAYERCORE_ADD_INT_LINK_LIB (geos_c)
    SET (wkbLibFlag "-lgeos -lgeos_c")
ELSE (HAVE_GEOS)
    TARGET_LINK_LIBRARIES (playerwkb playererror)
    SET (wkbLibFlag)
ENDIF (HAVE_GEOS)

PLAYER_MAKE_PKGCONFIG ("playerwkb" "Player WKB processing library - part of the Player Project" "playererror" "" "" "${wkbLibFlag}")
PLAYER_INSTALL_HEADERS (playerwkb playerwkb.h)
