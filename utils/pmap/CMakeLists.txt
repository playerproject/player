OPTION (BUILD_UTILS_PMAP "Build the pmap utility" ON)
IF (BUILD_UTILS_PMAP)
    INCLUDE (${PLAYER_CMAKE_DIR}/PlayerUtils.cmake)

    INCLUDE (FindPkgConfig)
    IF (NOT PKG_CONFIG_FOUND)
        MESSAGE (STATUS "WARNING: Could not find pkg-config; cannot build pmap utility.")
    ELSE (NOT PKG_CONFIG_FOUND)
        pkg_check_modules (GSL_PKG gsl)

        IF (GSL_PKG_FOUND)
            LIST_TO_STRING (GSL_CFLAGS "${GSL_PKG_CFLAGS}")
            LIST_TO_STRING (GSL_LDFLAGS "${GSL_PKG_LDFLAGS}")

            INCLUDE (FindOpenGL)
            INCLUDE (FindGLUT)

            SET (pmapconfig_h_in "${CMAKE_CURRENT_SOURCE_DIR}/pmapconfig.h.in")
            SET (pmapconfig_h "${CMAKE_CURRENT_BINARY_DIR}/pmapconfig.h")
            CONFIGURE_FILE (${pmapconfig_h_in} ${pmapconfig_h})

            SET (pmapSrcs logfile.cpp omap.cpp pmap.cpp rmap.cpp slap.cpp ${pmapconfig_h})
            SET (lodoSrcs lodo.cpp slap.cpp ${pmapconfig_h})
            SET (pmaptestSrcs pmap_test.cpp ${pmapconfig_h})
            SET (lododriverSrcs lodo_driver.cc)

            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs
                                 ${CMAKE_CURRENT_BINARY_DIR})
            IF (GLUT_FOUND)
                INCLUDE_DIRECTORIES (${GLUT_INCLUDE_DIR})
            ENDIF (GLUT_FOUND)

            PLAYER_ADD_EXECUTABLE (pmaptest ${pmaptestSrcs})
            TARGET_LINK_LIBRARIES (pmaptest pmap lodo)
            IF (GLUT_FOUND)
                TARGET_LINK_LIBRARIES (pmaptest ${GLUT_LIBRARIES})
            ENDIF (GLUT_FOUND)
            SET_TARGET_PROPERTIES (pmaptest PROPERTIES
                LINK_FLAGS "${GSL_LDFLAGS}")

            PLAYER_ADD_LIBRARY (pmap ${pmapSrcs})
            IF (GLUT_FOUND)
                TARGET_LINK_LIBRARIES (pmap ${GLUT_LIBRARIES})
            ENDIF (GLUT_FOUND)
            SET_SOURCE_FILES_PROPERTIES (${pmapSrcs} PROPERTIES
                COMPILE_FLAGS "${GSL_CFLAGS} --fast-math")
            SET_TARGET_PROPERTIES (pmap PROPERTIES
                LINK_FLAGS "${GSL_LDFLAGS}")

            PLAYER_ADD_LIBRARY (lodo ${lodoSrcs})
            TARGET_LINK_LIBRARIES (lodo playercore)
            IF (GLUT_FOUND)
                TARGET_LINK_LIBRARIES (lodo ${GLUT_LIBRARIES})
            ENDIF (GLUT_FOUND)
            SET_SOURCE_FILES_PROPERTIES (${lodoSrcs} PROPERTIES
                COMPILE_FLAGS "${GSL_CFLAGS} --fast-math")
            SET_TARGET_PROPERTIES (lodo PROPERTIES
                LINK_FLAGS "${GSL_LDFLAGS}")

            PLAYER_ADD_LIBRARY (lododriver ${lododriverSrcs})
            TARGET_LINK_LIBRARIES (lododriver lodo)
            IF (GLUT_FOUND)
                TARGET_LINK_LIBRARIES (lododriver ${GLUT_LIBRARIES})
            ENDIF (GLUT_FOUND)
        ELSE (GSL_PKG_FOUND)
            MESSAGE (STATUS "pmap utilities will not be built - GSL not found")
        ENDIF (GSL_PKG_FOUND)
    ENDIF (NOT PKG_CONFIG_FOUND)
ENDIF (BUILD_UTILS_PMAP)
