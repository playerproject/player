INCLUDE (PlayerUtils)
PROJECT (WavefrontTest)

SET (wavefrontSrcs ../test.c
                   ../plan.c
                   ../plan_plan.c
                   ../plan_waypoint.c
                   ../heap.c
                   ../plan_control.c)

INCLUDE (FindPkgConfig)
IF (NOT PKG_CONFIG_FOUND)
    MESSAGE (FATAL_ERROR "Could not find pkg-config - cannot search for gdk-pixbuf.")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (GDK_PKG gdk-pixbuf-2.0)
    IF (GDK_PKG_FOUND)
        IF (GDK_PKG_CFLAGS_OTHER)
            LIST_TO_STRING (GDK_CFLAGS "${GDK_PKG_CFLAGS_OTHER}")
        ENDIF (GDK_PKG_CFLAGS_OTHER)
        IF (GDK_PKG_LDFLAGS_OTHER)
            LIST_TO_STRING (GDK_LDFLAGS "${GDK_PKG_LDFLAGS_OTHER}")
        ENDIF (GDK_PKG_LDFLAGS_OTHER)
    ELSE (GDK_PKG_FOUND)
    ENDIF (GDK_PKG_FOUND)
ENDIF (NOT PKG_CONFIG_FOUND)

INCLUDE_DIRECTORIES (..)
IF (GDK_PKG_INCLUDE_DIRS)
    INCLUDE_DIRECTORIES (${GDK_PKG_INCLUDE_DIRS})
ENDIF (GDK_PKG_INCLUDE_DIRS)
IF (GDK_PKG_LIBRARY_DIRS)
    LINK_DIRECTORIES (${GDK_PKG_LIBRARY_DIRS})
ENDIF (GDK_PKG_LIBRARY_DIRS)
ADD_EXECUTABLE (test ${wavefrontSrcs})
TARGET_LINK_LIBRARIES (test ${GDK_PKG_LIBRARIES})
