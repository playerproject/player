# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

FILE (GLOB interfaceFiles ${CMAKE_CURRENT_SOURCE_DIR}/interfaces/*.def)

INCLUDE (${PLAYER_CMAKE_DIR}/internal/ProcessInterfaces.cmake)

# More cross-directory dependency stuff. Ug.
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

SET (playerxdr_h "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.h")
SET (playerxdr_c "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.c")
ADD_CUSTOM_COMMAND (OUTPUT ${playerxdr_h} ${playerxdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/playerxdrgen.py -distro ${CMAKE_CURRENT_SOURCE_DIR}/player.h ${playerxdr_c} ${playerxdr_h} ${player_interfaces_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles} ${player_interfaces_h}
)
ADD_CUSTOM_TARGET (playerxdr_src ALL
    DEPENDS ${playerxdr_h} ${playerxdr_c}
    COMMENT "Generating playerxdr.?")

SET (functiontable_gen_h "${CMAKE_CURRENT_BINARY_DIR}/functiontable_gen.h")
ADD_CUSTOM_COMMAND (OUTPUT ${functiontable_gen_h}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/playerinterfacegen.py --functiontable ${CMAKE_CURRENT_SOURCE_DIR}/interfaces > ${functiontable_gen_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles}
)
ADD_CUSTOM_TARGET (functiontable_gen ALL
    DEPENDS ${functiontable_gen_h}
    COMMENT "Generating functiontable_gen.h")

IF (NOT HAVE_XDR)
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
ENDIF (NOT HAVE_XDR)

SET (playerinterfaceSrcs  ${playerxdr_h}
                    ${playerxdr_c}
                    functiontable.c
                    addr_util.c
                    interface_util.c
                    ${functiontable_gen_h})

IF (PLAYER_OS_QNX)
    SET (interfaceLibFlag -lrpc)
ELSE (PLAYER_OS_QNX)
    SET (interfaceLibFlag)
ENDIF (PLAYER_OS_QNX)

INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})
TARGET_LINK_LIBRARIES(playercommon)
PLAYER_ADD_LIBRARY (playerinterface ${playerinterfaceSrcs})
IF (PLAYER_OS_WIN)
    TARGET_LINK_LIBRARIES (playerinterface Ws2_32)
ENDIF (PLAYER_OS_WIN)
SET_SOURCE_FILES_PROPERTIES (${playerxdr_h} ${playerxdr_c} PROPERTIES GENERATED TRUE)
IF (PLAYER_OS_QNX)
    TARGET_LINK_LIBRARIES (playerinterface rpc)
    PLAYERCORE_ADD_INT_LINK_LIB (rpc)
ENDIF (PLAYER_OS_QNX)
IF (NOT HAVE_XDR)
    TARGET_LINK_LIBRARIES (playerinterface playerreplace)
ENDIF (NOT HAVE_XDR)
PLAYER_MAKE_PKGCONFIG ("playerinterface" "Player Interface library - part of the Player Project" "" "" "" "${xdrLibFlag}")


PLAYER_INSTALL_HEADERS (playerinterface ${playerxdr_h} 
                                        addr_util.h 
                                        functiontable.h 
                                        interface_util.h 
                                        ${player_interfaces_h} 
                                        player.h)

INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/playerxdrgen.py RENAME playerxdrgen DESTINATION bin)
INSTALL (FILES ${CMAKE_CURRENT_SOURCE_DIR}/playerinterfacegen.py RENAME playerintefacegen DESTINATION bin)
