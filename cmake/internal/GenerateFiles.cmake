INCLUDE (${PLAYER_CMAKE_DIR}/internal/ProcessInterfaces.cmake)

SET (player_interfaces_h "${PROJECT_BINARY_DIR}/libplayercore/player_interfaces.h")
SET (interface_table_h "${PROJECT_BINARY_DIR}/libplayercore/interface_table.h")
PROCESS_INTERFACES ("" ${PROJECT_SOURCE_DIR}/libplayercore/interfaces ${player_interfaces_h})
PROCESS_INTERFACES ("--utils" ${PROJECT_SOURCE_DIR}/libplayercore/interfaces ${interface_table_h})

SET (playerxdr_h "${PROJECT_BINARY_DIR}/libplayerxdr/playerxdr.h")
SET (playerxdr_c "${PROJECT_BINARY_DIR}/libplayerxdr/playerxdr.c")
ADD_CUSTOM_COMMAND (OUTPUT ${playerxdr_h} ${playerxdr_c}
    COMMAND python ${PROJECT_SOURCE_DIR}/libplayerxdr/playerxdrgen.py -distro ${PROJECT_SOURCE_DIR}/libplayercore/player.h ${playerxdr_c} ${playerxdr_h} ${PROJECT_SOURCE_DIR}/libplayercore/player_interfaces.h
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/libplayerxdr
    DEPENDS ${interfaceFiles} ${PROJECT_SOURCE_DIR}/libplayercore/player_interfaces.h
    VERBATIM
)
