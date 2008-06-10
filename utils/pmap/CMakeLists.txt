INCLUDE (${PLAYER_CMAKE_DIR}/PlayerUtils.cmake)

INCLUDE (FindPkgConfig)
IF (NOT PKG_CONFIG_FOUND)
    MESSAGE (STATUS "WARNING: Could not find pkg-config; cannot build pmap utility.")
ELSE (NOT PKG_CONFIG_FOUND)
    pkg_check_modules (GSL_PKG gsl)

    IF (GSL_PKG_FOUND)
        LIST_TO_STRING (GSL_CFLAGS "${GSL_PKG_CFLAGS}")
        LIST_TO_STRING (GSL_LDFLAGS "${GSL_PKG_LDFLAGS}")

        SET (pmapSrcs logfile.cpp omap.cpp pmap.cpp rmap.cpp slap.cpp)
        SET (lodoSrcs lodo.cpp slap.cpp)
        SET (pmaptestSrcs pmap_test.cpp)
        SET (lododriverSrcs lodo_driver.cc)

        INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

        PLAYER_ADD_EXECUTABLE (pmaptest ${pmaptestSrcs})
        TARGET_LINK_LIBRARIES (pmaptest pmap lodo)
        SET_TARGET_PROPERTIES (pmaptest PROPERTIES
            LINK_FLAGS "${GSL_LDFLAGS}")

        PLAYER_ADD_LIBRARY (pmap ${pmapSrcs})
        SET_SOURCE_FILES_PROPERTIES (${pmapSrcs} PROPERTIES
            COMPILE_FLAGS "${GSL_CFLAGS} --fast-math")
        SET_TARGET_PROPERTIES (pmap PROPERTIES
            LINK_FLAGS "${GSL_LDFLAGS}")

        PLAYER_ADD_LIBRARY (lodo ${lodoSrcs})
        TARGET_LINK_LIBRARIES (lodo playercore)
        SET_SOURCE_FILES_PROPERTIES (${lodoSrcs} PROPERTIES
            COMPILE_FLAGS "${GSL_CFLAGS} --fast-math")
        SET_TARGET_PROPERTIES (lodo PROPERTIES
            LINK_FLAGS "${GSL_LDFLAGS}")

        PLAYER_ADD_LIBRARY (lododriver ${lododriverSrcs})
        TARGET_LINK_LIBRARIES (lododriver lodo)
    ELSE (GSL_PKG_FOUND)
        MESSAGE (STATUS "pmap utilities will not be built - GSL not found")
    ENDIF (GSL_PKG_FOUND)
ENDIF (NOT PKG_CONFIG_FOUND)
