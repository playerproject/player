INCLUDE (FindPythonInterp)
IF (PYTHONINTERP_FOUND)
    OPTION (BUILD_PYTHONC_BINDINGS "Build the Python bindings for the C client library" ON)

    IF (BUILD_PYTHONC_BINDINGS)
        FIND_PACKAGE (SWIG)
        IF (SWIG_FOUND)
            MESSAGE (STATUS "Python bindings for C client library will be built")

            #INCLUDE (${SWIG_USE_FILE})
            INCLUDE (${PLAYER_CMAKE_DIR}/internal/UseSwig_Player.cmake)

            FIND_PACKAGE (PythonLibs)
            INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR})
            INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR})
            INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs/libplayerc)

            SET (CMAKE_SWIG_FLAGS "")

            # Generate playerc_wrap.i and playerc_wrap.h from playerc.h
            SET (playerc_wrap_prefix "${CMAKE_CURRENT_BINARY_DIR}/playerc_wrap")
            ADD_CUSTOM_COMMAND (OUTPUT "${playerc_wrap_prefix}.i"
                COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/playerc_swig_parse.py ${PROJECT_SOURCE_DIR}/client_libs/libplayerc/playerc.h ${playerc_wrap_prefix}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                VERBATIM)

            ADD_CUSTOM_COMMAND(OUTPUT "${playerc_wrap_prefix}.h"
                DEPENDS "${playerc_wrap_prefix}.i")

            # Empty target to ensure playerc_wrap.i and playerc_wrap.h get created
            ADD_CUSTOM_TARGET (playerc_wrap_i_target ALL
                DEPENDS "${playerc_wrap_prefix}.i"
                DEPENDS "${playerc_wrap_prefix}.h")

            # Copy playerc.i to the build directory
            SET (playerc_i "${CMAKE_CURRENT_SOURCE_DIR}/playerc.i")

            SET (SWIG_MODULE_playerc_EXTRA_DEPS ${PROJECT_SOURCE_DIR}/client_libs/libplayerc/playerc.h
                                                ${PROJECT_BINARY_DIR}/libplayerinterface/player_interfaces.h)
            SWIG_ADD_MODULE (playerc python ${playerc_i})
            SWIG_LINK_LIBRARIES (playerc ${PYTHON_LIBRARIES})
            ADD_DEPENDENCIES (${SWIG_MODULE_playerc_REAL_NAME} playerc_wrap_i_target)
            TARGET_LINK_LIBRARIES (${SWIG_MODULE_playerc_REAL_NAME} playerinterface playerc playercommon playerwkb)
            IF (HAVE_JPEG)
                TARGET_LINK_LIBRARIES (${SWIG_MODULE_playerc_REAL_NAME} playerjpeg)
            ENDIF (HAVE_JPEG)

            IF (PYTHON_OS_WIN)
                GET_FILENAME_COMPONENT (playercpyInstallDir ${PYTHON_EXECUTABLE} PATH)
            ELSE (PYTHON_OS_WIN)
                # Get the Python version
                EXECUTE_PROCESS (COMMAND ${PYTHON_EXECUTABLE} -V
                                 ERROR_VARIABLE pythonVersionString
                                 ERROR_STRIP_TRAILING_WHITESPACE)
                STRING (REGEX REPLACE "^Python ([0-9]+\\.[0-9]+).*" "\\1" pythonVersion ${pythonVersionString})
                SET (playercpyInstallDir ${PLAYER_LIBRARY_INSTALL_DIR}/python${pythonVersion}/site-packages)
            ENDIF (PYTHON_OS_WIN)
            SET (PYTHON_BINDINGS_INSTALL_DIR ${playercpyInstallDir} CACHE PATH "Python bindings installation directory under $prefix")
            MARK_AS_ADVANCED (PYTHON_BINDINGS_INSTALL_DIR)

            INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/playerc.py ${CMAKE_CURRENT_BINARY_DIR}/_playerc.so
                DESTINATION ${PYTHON_BINDINGS_INSTALL_DIR})
        ELSE (SWIG_FOUND)
            MESSAGE (STATUS "Python bindings for C client library will not be built - could not find Swig")
        ENDIF (SWIG_FOUND)
    ELSE (BUILD_PYTHONC_BINDINGS)
        MESSAGE (STATUS "Python bindings for C client library will not be built - disabled by user")
    ENDIF (BUILD_PYTHONC_BINDINGS)
ELSE (PYTHONINTERP_FOUND)
    MESSAGE (STATUS "Python bindings for C client library will not be built - could not find Python")
ENDIF (PYTHONINTERP_FOUND)

