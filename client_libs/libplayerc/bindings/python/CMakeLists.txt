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
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR})
            INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR})
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs/libplayerc)

            SET (CMAKE_SWIG_FLAGS "")

            # Generate player_oo.i from playerc.h
            SET (playerc_oo_i "${CMAKE_CURRENT_BINARY_DIR}/playerc_oo.i")
            ADD_CUSTOM_COMMAND (OUTPUT ${playerc_oo_i}
                COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/playerc_swig_parse.py ${PROJECT_SOURCE_DIR}/client_libs/libplayerc/playerc.h ${playerc_oo_i}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                VERBATIM
            )
            # Empty target to ensure playerc_oo.i gets created
            ADD_CUSTOM_TARGET (playerc_oo_i_target ALL
                DEPENDS ${playerc_oo_i})
            # Copy playerc.i to the build directory
            SET (playerc_i_in "${CMAKE_CURRENT_SOURCE_DIR}/playerc.i")
            SET (playerc_i "${CMAKE_CURRENT_BINARY_DIR}/playerc.i")
            CONFIGURE_FILE (${playerc_i_in} ${playerc_i} COPYONLY)

            SWIG_ADD_MODULE (playerc python ${playerc_i})
            SWIG_LINK_LIBRARIES (playerc ${PYTHON_LIBRARIES})
            ADD_DEPENDENCIES (${SWIG_MODULE_playerc_REAL_NAME} playerc_oo_i_target)
            TARGET_LINK_LIBRARIES (${SWIG_MODULE_playerc_REAL_NAME} playerxdr playerc playerjpeg jpeg playererror)

            # Generate the set up script
#             CONFIGURE_FILE (${CMAKE_CURRENT_SOURCE_DIR}/setup.py.cmake ${CMAKE_CURRENT_BINARY_DIR}/setup.py)
#             INSTALL (SCRIPT ${PLAYER_CMAKE_DIR}/internal/InstallPythonModule.cmake)

            INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/playerc.py ${CMAKE_CURRENT_BINARY_DIR}/_playerc.so
                DESTINATION lib/python/site-packages)
        ELSE (SWIG_FOUND)
            MESSAGE (STATUS "Python bindings for C client library will not be built - could not find Swig")
        ENDIF (SWIG_FOUND)
    ELSE (BUILD_PYTHON_BINDINGS)
        MESSAGE (STATUS "Python bindings for C client library will not be built - disabled by user")
    ENDIF (BUILD_PYTHON_BINDINGS)
ELSE (PYTHONINTERP_FOUND)
    MESSAGE (STATUS "Python bindings for C client library will not be built - could not find Python")
ENDIF (PYTHONINTERP_FOUND)
