# The Python interpreter is necessary to compile this library
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

FILE (GLOB interfaceFiles ${PROJECT_SOURCE_DIR}/libplayercore/interfaces/*.def)

# More cross-directory dependency stuff. Ug.
SET (player_interfaces_h "${CMAKE_CURRENT_BINARY_DIR}/player_interfaces.h")
PROCESS_INTERFACES ("" ${PROJECT_SOURCE_DIR}/libplayercore/interfaces ${player_interfaces_h})

SET (playerxdr_h "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.h")
SET (playerxdr_c "${CMAKE_CURRENT_BINARY_DIR}/playerxdr.c")
ADD_CUSTOM_COMMAND (OUTPUT ${playerxdr_h} ${playerxdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/playerxdrgen.py -distro ${PROJECT_SOURCE_DIR}/libplayercore/player.h ${playerxdr_c} ${playerxdr_h} ${player_interfaces_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles} ${player_interfaces_h}
)
ADD_CUSTOM_TARGET (playerxdr_src ALL
    DEPENDS ${playerxdr_h} ${playerxdr_c}
    COMMENT "Generating playerxdr.?")

SET (functiontable_gen_h "${CMAKE_CURRENT_BINARY_DIR}/functiontable_gen.h")
ADD_CUSTOM_COMMAND (OUTPUT ${functiontable_gen_h}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayercore/playerinterfacegen.py --functiontable ${PROJECT_SOURCE_DIR}/libplayercore/interfaces > ${functiontable_gen_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS ${interfaceFiles}
)
ADD_CUSTOM_TARGET (functiontable_gen ALL
    DEPENDS ${functiontable_gen_h}
    COMMENT "Generating functiontable_gen.h")

SET (playerxdrSrcs  ${playerxdr_h}
                    ${playerxdr_c}
                    functiontable.c
                    ${functiontable_gen_h})

IF (PLAYER_OS_QNX)
    SET (xdrLibFlag -lrpc)
ELSE (PLAYER_OS_QNX)
    SET (xdrLibFlag)
ENDIF (PLAYER_OS_QNX)

INCLUDE_DIRECTORIES (${CMAKE_CURRENT_BINARY_DIR})
PLAYER_ADD_LIBRARY (playerxdr ${playerxdrSrcs})
SET_SOURCE_FILES_PROPERTIES (${playerxdr_h} ${playerxdr_c} PROPERTIES GENERATED TRUE)
IF (PLAYER_OS_QNX)
    TARGET_LINK_LIBRARIES (playerxdr rpc)
    PLAYER_ADD_LINK_LIB (rpc)
ENDIF (PLAYER_OS_QNX)
PLAYER_MAKE_PKGCONFIG ("playerxdr" "Player XDR messaging library - part of the Player Project" "" "" "" "${xdrLibFlag}")

PLAYER_INSTALL_HEADERS (playerxdr ${playerxdr_h} functiontable.h)
