# Processes the interface files in the specified directory into the provided output file name

MACRO (PROCESS_INTERFACES options directory outputFile)
    FILE (GLOB INTERFACE_DEPENDENCIES ${directory}/*.def)
    ADD_CUSTOM_COMMAND (OUTPUT ${outputFile}
        COMMAND python ${PROJECT_SOURCE_DIR}/libplayercore/playerinterfacegen.py ${options} ${directory} > ${outputFile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${INTERFACE_DEPENDENCIES} ${ARGN}
        VERBATIM
    )
ENDMACRO (PROCESS_INTERFACES outputFile directory)