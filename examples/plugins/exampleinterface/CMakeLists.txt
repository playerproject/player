INCLUDE_DIRECTORIES (${PLAYERCORE_EXTRA_INCLUDE_DIRS})
LINK_DIRECTORIES (${PLAYERCORE_EXTRA_LINK_DIRS})
INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

SET (example_interface_h ${CMAKE_CURRENT_BINARY_DIR}/example_interface.h)
ADD_CUSTOM_COMMAND (OUTPUT ${example_interface_h}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerinterface/playerinterfacegen.py --plugin 128_example.def > ${example_interface_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

SET (example_functiontable_c ${CMAKE_CURRENT_BINARY_DIR}/example_functiontable.c)
ADD_CUSTOM_COMMAND (OUTPUT ${example_functiontable_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerinterface/playerinterfacegen.py --plugin --functiontable 128_example.def > ${example_functiontable_c}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
)

SET (example_xdr_h ${CMAKE_CURRENT_BINARY_DIR}/example_xdr.h)
SET (example_xdr_c ${CMAKE_CURRENT_BINARY_DIR}/example_xdr.c)
ADD_CUSTOM_COMMAND (OUTPUT ${example_xdr_h} ${example_xdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerinterface/playerxdrgen.py ${example_interface_h} ${example_xdr_c} ${example_xdr_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/example_interface.h
)

INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/client_libs ${CMAKE_CURRENT_BINARY_DIR})
ADD_LIBRARY (example SHARED ${example_interface_h} ${example_xdr_h} ${example_xdr_c} eginterf_client.c ${example_functiontable_c})
TARGET_LINK_LIBRARIES (example playerc)
GET_SOURCE_FILE_PROPERTY (currentCFlags ${example_xdr_c} COMPILE_FLAGS)
IF (currentCFlags STREQUAL "NOTFOUND")
    SET (newCFlags "-Dplayerxdr_EXPORTS")
ELSE (currentCFlags STREQUAL "NOTFOUND")
    SET (newCFlags "-Dplayerxdr_EXPORTS ${currentCFlags}")
ENDIF (currentCFlags STREQUAL "NOTFOUND")
SET_SOURCE_FILES_PROPERTIES (${example_xdr_c} PROPERTIES COMPILE_FLAGS  ${newCFlags})
ADD_LIBRARY (example_driver SHARED ${example_interface_h} ${example_xdr_c} ${example_xdr_h} eginterf_driver.cc)
TARGET_LINK_LIBRARIES (example_driver playercore)
ADD_EXECUTABLE (example_client example_client.c ${example_interface_h})
TARGET_LINK_LIBRARIES (example_client playerc playerinterface playercommon ${PLAYERC_EXTRA_LINK_LIBRARIES} example)

SET (exampleCMakeLists_in ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.example.in)
SET (exampleCMakeLists ${CMAKE_CURRENT_BINARY_DIR}/CMakeLists.txt.example)
SET (exampleDestDir share/${PROJECT_NAME_LOWER}/examples/plugins/exampleinterface)
CONFIGURE_FILE (${exampleCMakeLists_in} ${exampleCMakeLists} @ONLY)
INSTALL (FILES ${exampleCMakeLists} DESTINATION ${exampleDestDir} RENAME CMakeLists.txt COMPONENT examples)
INSTALL (FILES 128_example.def eginterf_client.c eginterf_client.h eginterf_driver.cc example.cfg example_client.c README
    DESTINATION ${exampleDestDir} COMPONENT examples)
