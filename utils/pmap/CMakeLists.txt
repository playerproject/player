INCLUDE (${PLAYER_CMAKE_DIR}/internal/Utils.cmake)

CHECK_PACKAGE_EXISTS (gsl haveGSL gslIncludeDir gslLibDir gslLinkFlags gslCFlags)
# The GSL pkg-config gives an almost-empty cflags, containing only a new line.
# The pkg-config module for cmake doesn't strip trailing white space on the output
# of executing the pkg-config command, so this new line makes it into the cflags
# and screws up the generated make files.
SET (gslCFlags)

IF (haveGSL)
    SET (pmapSrcs logfile.cpp omap.cpp pmap.cpp rmap.cpp slap.cpp)
    SET (lodoSrcs lodo.cpp)
    SET (pmaptestSrcs pmap_test.cpp)
    SET (lododriverSrcs lodo_driver.cc)

    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

    PLAYER_ADD_EXECUTABLE (pmaptest ${pmaptestSrcs})
    TARGET_LINK_LIBRARIES (pmaptest pmap lodo)
    SET_TARGET_PROPERTIES (pmaptest PROPERTIES
        LINK_FLAGS "${gslLinkFlags}")

    PLAYER_ADD_LIBRARY (pmap ${pmapSrcs})
    SET_SOURCE_FILES_PROPERTIES (${pmapSrcs} PROPERTIES
        COMPILE_FLAGS "${gslCFlags} --fast-math")

    PLAYER_ADD_LIBRARY (lodo ${lodoSrcs})
    SET_SOURCE_FILES_PROPERTIES (${lodoSrcs} PROPERTIES
        COMPILE_FLAGS "${gslCFlags} --fast-math")
    SET_TARGET_PROPERTIES (lodo PROPERTIES
        LINK_FLAGS "${gslLinkFlags}")

    PLAYER_ADD_LIBRARY (lododriver ${lododriverSrcs})
    TARGET_LINK_LIBRARIES (lododriver lodo)
ELSE (haveGSL)
    MESSAGE (STATUS "pmap utilities will not be built - GSL not found")
ENDIF (haveGSL)