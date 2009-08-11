# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

IF (NOT PTHREAD_INCLUDE_DIR STREQUAL "")
    PLAYERCORE_ADD_INCLUDE_DIR (${PTHREAD_INCLUDE_DIR})
ENDIF (NOT PTHREAD_INCLUDE_DIR STREQUAL "")
IF (NOT PTHREAD_LIB_DIR STREQUAL "")
    PLAYERCORE_ADD_LINK_DIR (${PTHREAD_LIB_DIR})
    SET (playercoreLinkDirs ${playercoreLinkDirs} ${PTHREAD_LIB_DIR})
ENDIF (NOT PTHREAD_LIB_DIR STREQUAL "")

SET (playercoreSrcs driver.cc
                    device.cc
                    drivertable.cc
                    devicetable.cc
                    configfile.cc
                    filewatcher.cc
                    message.cc
                    wallclocktime.cc
                    plugins.cc
                    globals.cc
                    property.cpp
                    threaded_driver.cc
                    remote_driver.cc)

IF (NOT PLAYER_OS_QNX)
    PLAYERCORE_ADD_LINK_LIB (${PTHREAD_LIB})
ENDIF (NOT PLAYER_OS_QNX)

IF (HAVE_CLOCK_GETTIME)
    PLAYERCORE_ADD_INT_LINK_LIB (rt)
    SET (playercoreLinkLibs ${playercoreLinkLibs} "rt")
ELSE (HAVE_CLOCK_GETTIME)
    SET (playerreplaceLib playerreplace)
ENDIF (HAVE_CLOCK_GETTIME)
IF (NOT HAVE_GETTIMEOFDAY)
    SET (playerreplaceLib playerreplace)
ENDIF (NOT HAVE_GETTIMEOFDAY)
IF (HAVE_LIBLTDL)
    IF (PLAYER_OS_QNX)
        PLAYERCORE_ADD_LINK_LIB (ltdl)
        SET (playercoreLinkLibs ${playercoreLinkLibs} "ltdl")
    ELSE (PLAYER_OS_QNX)
        PLAYERCORE_ADD_LINK_LIB (ltdl dl)
        SET (playercoreLinkLibs ${playercoreLinkLibs} "ltdl" "dl")
    ENDIF (PLAYER_OS_QNX)
ENDIF (HAVE_LIBLTDL)
IF (NOT HAVE_DIRNAME)
    SET (playerreplaceLib playerreplace)
ENDIF (NOT HAVE_DIRNAME)

PLAYERCORE_ADD_INT_LINK_DIR (${CMAKE_BINARY_DIR}/libplayerinterface ${CMAKE_BINARY_DIR}/libplayercommon)
PLAYERCORE_ADD_INT_LINK_LIB (playerinterface playercommon)

INCLUDE_DIRECTORIES (${PLAYERCORE_INT_INCLUDE_DIRS} ${PLAYERCORE_EXTRA_INCLUDE_DIRS})
LINK_DIRECTORIES (${PLAYERCORE_INT_LINK_DIRS} ${PLAYERCORE_EXTRA_LINK_DIRS})
PLAYER_ADD_LIBRARY (playercore ${playercoreSrcs})
TARGET_LINK_LIBRARIES (playercore ${playerreplaceLib}
                                  ${PLAYERCORE_INT_LINK_LIBRARIES}
                                  ${PLAYERCORE_EXTRA_LINK_LIBRARIES})
SET (pkgconfigCFlags)
SET (pkgconfigLinkDirs)
SET (pkgconfigLinkLibs)
LIST_TO_STRING_WITH_PREFIX (pkgconfigCFlags "-I" ${PLAYERCORE_EXTRA_INCLUDE_DIRS})
LIST_TO_STRING_WITH_PREFIX (pkgconfigLinkDirs "-L" ${PLAYERCORE_EXTRA_LINK_DIRS})
LIST_TO_STRING_WITH_PREFIX (pkgconfigLinkLibs "-l" ${PLAYERCORE_EXTRA_LINK_LIBRARIES})
PLAYER_MAKE_PKGCONFIG ("playercore" "Player core library - part of the Player Project"
                       "playerinterface playercommon" "" "${pkgconfigCFlags}"
                       "${pkgconfigLinkDirs} ${pkgconfigLinkLibs}")
CONFIGURE_FILE (${PLAYER_CMAKE_DIR}/UsePlayerPlugin.cmake.in ${CMAKE_BINARY_DIR}/cmake/UsePlayerPlugin.cmake @ONLY)

PLAYER_INSTALL_HEADERS (playercore configfile.h
                                   device.h
                                   devicetable.h
                                   driver.h
                                   drivertable.h
                                   filewatcher.h
                                   globals.h
                                   message.h
                                   playercore.h
                                   playertime.h
                                   plugins.h
                                   property.h
                                   wallclocktime.h)

