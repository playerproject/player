INCLUDE (${PLAYER_CMAKE_DIR}/internal/ProcessInterfaces.cmake)

# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

SET (player_interfaces_h "${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h")
SET (interface_table_h "${CMAKE_CURRENT_BINARY_DIR}/interface_table.h")
PROCESS_INTERFACES ("" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${player_interfaces_h})
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
    DEPENDS ${interfaceFiles} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h
)

# Include from the binary dir to get player_interfaces.h and interface_table.h
INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})

SET (playercore_srcs    driver.cc
                        device.cc
                        drivertable.cc
                        devicetable.cc
                        configfile.cc
                        message.cc
                        wallclocktime.cc
                        plugins.cc
                        globals.cc
                        property.cpp)
PLAYER_ADD_LIBRARY (playercore ${playercore_srcs} ${player_interfaces_h} ${playerxdr_h})
# TODO: playerxdr should NOT be linked here; it's a bogus dependency coming
# from the fact that message cloning functions are auto-generated into
# playerxdr and used here.  Those functions should go into a separate
# library.
TARGET_LINK_LIBRARIES (playercore playerutils playererror playerxdr ltdl dl pthread rt)
PLAYER_ADD_LINK_LIB (ltdl dl pthread rt)
PLAYER_MAKE_PKGCONFIG ("playercore" "Player core library - part of the Player Project" "playererror" "" "" "-lltdl -lpthread -lrt")


SET (playererror_srcs   error.c)
PLAYER_ADD_LIBRARY (playererror ${playererror_srcs})
PLAYER_MAKE_PKGCONFIG ("playererror" "Player error reporting library - part of the Player Project" "" "" "" "")


SET (playerutils_srcs   interface_util.c
                        addr_util.c)
PLAYER_ADD_LIBRARY (playerutils ${playerutils_srcs} ${interface_table_h})
TARGET_LINK_LIBRARIES (playerutils playererror)
PLAYER_MAKE_PKGCONFIG ("playerutils" "Player utilities library - part of the Player Project" "" "" "" "")

PLAYER_INSTALL_HEADERS (playercore addr_util.h
                                   configfile.h
                                   device.h
                                   devicetable.h
                                   driver.h
                                   drivertable.h
                                   error.h
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
