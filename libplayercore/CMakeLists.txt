INCLUDE (${PLAYER_CMAKE_DIR}/internal/ProcessInterfaces.cmake)

# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

SET (player_interfaces_h "${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h")
SET (interface_table_h "${CMAKE_CURRENT_BINARY_DIR}/interface_table.h")
PROCESS_INTERFACES ("" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${player_interfaces_h})
PROCESS_INTERFACES ("--utils" ${CMAKE_CURRENT_SOURCE_DIR}/interfaces ${interface_table_h} ${player_interfaces_h})

# For now, we're generating our own copy of playerxdr.h (and .c as a side-effect) here.
# This can be fixed when the cross-directory dependency with libplayerxdr is removed.
SET (playerxdr_h "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.h")
SET (playerxdr_c "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.c")
ADD_CUSTOM_COMMAND (OUTPUT ${playerxdr_h} ${playerxdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerxdr/playerxdrgen.py -distro ${CMAKE_CURRENT_SOURCE_DIR}/player.h ${playerxdr_c} ${playerxdr_h} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles} ${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h
    VERBATIM
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
TARGET_LINK_LIBRARIES (playercore ltdl dl pthread)
PLAYER_ADD_LINK_LIB (ltdl dl pthread)
PLAYER_MAKE_PKGCONFIG ("playercore" "Player core library - part of the Player Project" "playererror" "" "" "-lltdl -lpthread")


SET (playererror_srcs   error.c)
PLAYER_ADD_LIBRARY (playererror ${playererror_srcs})
PLAYER_MAKE_PKGCONFIG ("playererror" "Player error reporting library - part of the Player Project" "" "" "" "")


SET (playerutils_srcs   interface_util.c
                        addr_util.c)
PLAYER_ADD_LIBRARY (playerutils ${playerutils_srcs} ${interface_table_h})
PLAYER_MAKE_PKGCONFIG ("playerutils" "Player utilities library - part of the Player Project" "" "" "" "")