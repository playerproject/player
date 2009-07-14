# Processes the interface files in the specified directory into the provided output file name

# The Python interpreter is necessary
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

MACRO (PROCESS_INTERFACES options directory outputFile)
    ADD_CUSTOM_COMMAND (OUTPUT ${outputFile}
        COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerinterface/playerinterfacegen.py ${options} ${directory} > ${outputFile}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${ARGN})
ENDMACRO (PROCESS_INTERFACES outputFile directory)
