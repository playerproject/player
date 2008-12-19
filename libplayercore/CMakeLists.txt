INCLUDE (${PLAYER_CMAKE_DIR}/internal/ProcessInterfaces.cmake)

# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

FILE (GLOB interfaceFiles ${PROJECT_SOURCE_DIR}/libplayercore/interfaces/*.def)

SET (player_interfaces_h "${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h")
SET (interface_table_h "${CMAKE_CURRENT_BINARY_DIR}/interface_table.h")
PROCESS_INTERFACES ("" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${player_interfaces_h} ${interfaceFiles})
ADD_CUSTOM_TARGET (player_interfaces ALL
    DEPENDS ${player_interfaces_h}
    COMMENT "Generating player_interfaces.h")
PROCESS_INTERFACES ("--utils" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${interface_table_h} ${player_interfaces_h})
ADD_CUSTOM_TARGET (interface_table ALL
    DEPENDS ${interface_table_h}
    COMMENT "Generating interface_table.h")

# For now, we're generating our own copy of playerxdr.h (and .c as a side-effect) here.
# This can be fixed when the cross-directory dependency with libplayerxdr is removed.
SET (playerxdr_h "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.h")
SET (playerxdr_c "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.c")
ADD_CUSTOM_COMMAND (OUTPUT ${playerxdr_h} ${playerxdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerxdr/playerxdrgen.py -distro ${CMAKE_CURRENT_SOURCE_DIR}/player.h ${playerxdr_c} ${playerxdr_h} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h)

# Include from the binary dir to get player_interfaces.h and interface_table.h
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})

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
                    threaded_driver.cc)

# TODO: playerxdr should NOT be linked here; it's a bogus dependency coming
# from the fact that message cloning functions are auto-generated into
# playerxdr and used here.  Those functions should go into a separate
# library.
SET (coreLibs playerutils playererror playerxdr)
IF (PLAYER_OS_QNX)
    SET (corePCLibs)
ELSE (PLAYER_OS_QNX)
    SET (coreLibs ${coreLibs} pthread)
    SET (corePCLibs "-lpthread")
    PLAYER_ADD_LINK_LIB (pthread)
ENDIF (PLAYER_OS_QNX)

IF (HAVE_CLOCK_GETTIME)
    SET (coreLibs ${coreLibs} rt)
    SET (corePCLibs "${corePCLibs} -lrt")
    PLAYER_ADD_LINK_LIB (rt)
ELSE (HAVE_CLOCK_GETTIME)
    SET (playercoreSrcs ${playercoreSrcs} ${PROJECT_SOURCE_DIR}/replace/clock_gettime.c)
ENDIF (HAVE_CLOCK_GETTIME)
IF (HAVE_LIBLTDL)
    IF (PLAYER_OS_QNX)
        SET (coreLibs ${coreLibs} ltdl)
        SET (corePCLibs "${corePCLibs} -lltdl")
        PLAYER_ADD_LINK_LIB (ltdl)
    ELSE (PLAYER_OS_QNX)
        SET (coreLibs ${coreLibs} ltdl dl)
        SET (corePCLibs "${corePCLibs} -lltdl -ldl")
        PLAYER_ADD_LINK_LIB (ltdl dl)
    ENDIF (PLAYER_OS_QNX)
ENDIF (HAVE_LIBLTDL)
IF (NOT HAVE_DIRNAME)
  SET (playercoreSrcs ${playercoreSrcs} ${PROJECT_SOURCE_DIR}/replace/dirname.c)
ENDIF (NOT HAVE_DIRNAME)

PLAYER_ADD_LIBRARY (playercore ${playercoreSrcs} ${player_interfaces_h} ${playerxdr_h})
TARGET_LINK_LIBRARIES (playercore ${coreLibs})
PLAYER_MAKE_PKGCONFIG ("playercore" "Player core library - part of the Player Project"
                       "playererror" "" "" "${corePCLibs}")


SET (playererrorSrcs error.c)
PLAYER_ADD_LIBRARY (playererror ${playererrorSrcs})
PLAYER_MAKE_PKGCONFIG ("playererror" "Player error reporting library - part of the Player Project"
                       "" "" "" "")


SET (playerutilsSrcs interface_util.c
                      addr_util.c)
PLAYER_ADD_LIBRARY (playerutils ${playerutilsSrcs} ${interface_table_h})
TARGET_LINK_LIBRARIES (playerutils playererror)
PLAYER_MAKE_PKGCONFIG ("playerutils" "Player utilities library - part of the Player Project"
                       "" "" "" "")

PLAYER_INSTALL_HEADERS (playercore addr_util.h
                                   configfile.h
                                   device.h
                                   devicetable.h
                                   driver.h
                                   drivertable.h
                                   error.h
                                   filewatcher.h
                                   globals.h
                                   interface_util.h
                                   message.h
                                   playercommon.h
                                   ${playerconfig_h}
                                   playercore.h
                                   player.h
                                   ${player_interfaces_h}
                                   playertime.h
                                   plugins.h
                                   property.h
                                   wallclocktime.h)
