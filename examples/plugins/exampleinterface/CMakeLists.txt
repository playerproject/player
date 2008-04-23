INCLUDE (FindPythonInterp)
IF (NOT PYTHONINTERP_FOUND)
    MESSAGE (FATAL_ERROR "No Python interpreter found. Cannot continue.")
ENDIF (NOT PYTHONINTERP_FOUND)

SET (example_interface_h ${CMAKE_CURRENT_BINARY_DIR}/example_interface.h)
ADD_CUSTOM_COMMAND (OUTPUT ${example_interface_h}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayercore/playerinterfacegen.py --plugin 128_example.def > ${example_interface_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

SET (example_functiontable_c ${CMAKE_CURRENT_BINARY_DIR}/example_functiontable.c)
ADD_CUSTOM_COMMAND (OUTPUT ${example_functiontable_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayercore/playerinterfacegen.py --plugin --functiontable 128_example.def > ${example_functiontable_c}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS
)

SET (example_xdr_h ${CMAKE_CURRENT_BINARY_DIR}/example_xdr.h)
SET (example_xdr_c ${CMAKE_CURRENT_BINARY_DIR}/example_xdr.c)
ADD_CUSTOM_COMMAND (OUTPUT ${example_xdr_h} ${example_xdr_c}
    COMMAND ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/libplayerxdr/playerxdrgen.py ${example_interface_h} ${example_xdr_c} ${example_xdr_h}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/example_interface.h
)

INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR} ${PROJECT_SOURCE_DIR}/client_libs ${CMAKE_CURRENT_BINARY_DIR})
ADD_LIBRARY (eginterf SHARED ${example_interface_h} ${example_xdr_h} ${example_xdr_c} eginterf_client.c ${example_functiontable_c})
TARGET_LINK_LIBRARIES (eginterf playerc)
ADD_LIBRARY (eginterfdriver SHARED ${example_interface_h} ${example_xdr_h} eginterf_driver.cc)
TARGET_LINK_LIBRARIES (eginterfdriver playercore)
ADD_EXECUTABLE (example_client example_client.c ${example_interface_h})
TARGET_LINK_LIBRARIES (example_client playerc playerxdr playererror ${PLAYERC_EXTRA_LINK_LIBRARIES} eginterf)
