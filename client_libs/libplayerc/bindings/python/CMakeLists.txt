INCLUDE (FindPythonInterp)
IF (PYTHONINTERP_FOUND)
    OPTION (BUILD_PYTHON_BINDINGS "Build the Python bindings for the C client library" ON)

    IF (BUILD_PYTHON_BINDINGS)
        FIND_PACKAGE (SWIG)
        IF (SWIG_FOUND)
            MESSAGE (STATUS "Python bindings for C client library will be built")

            INCLUDE (${SWIG_USE_FILE})

            FIND_PACKAGE (PythonLibs)
            INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})

            INCLUDE_DIRECTORIES (${CMAKE_CURRENT_SOURCE_DIR})

            SET (CMAKE_SWIG_FLAGS "")

            # Generate player_oo.i from playerc.h
            SET (player_oo_i "${CMAKE_CURRENT_BINARY_DIR}/player_oo.i")
            ADD_CUSTOM_COMMAND (OUTPUT ${player_oo_i}
                COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/playerc_swig_parse.py ${PROJECT_SOURCE_DIR}/client_libs/libplayerc/playerc.h ${player_oo_i}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                VERBATIM
            )

            SWIG_ADD_MODULE (playerc python ${player_oo_i})
            SWIG_LINK_LIBRARIES (playerc ${PYTHON_LIBRARIES})

            # Generate the set up script
            CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/setup.py.cmake ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
        ELSE (SWIG_FOUND)
            MESSAGE (STATUS "Python bindings for C client library will not be built - could not find Swig")
        ENDIF (SWIG_FOUND)
    ELSE (BUILD_PYTHON_BINDINGS)
        MESSAGE (STATUS "Python bindings for C client library will not be built - disabled by user")
    ENDIF (BUILD_PYTHON_BINDINGS)
ELSE (PYTHONINTERP_FOUND)
    MESSAGE (STATUS "Python bindings for C client library will not be built - could not find Python")
ENDIF (PYTHONINTERP_FOUND)
