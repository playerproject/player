INCLUDE (FindPythonInterp)

IF(BUILD_PLAYERCC)
    IF (PYTHONINTERP_FOUND)
        OPTION (BUILD_PYTHONCPP_BINDINGS "Build the Python bindings for the C++ client library" OFF)

        IF (BUILD_PYTHONCPP_BINDINGS)
            FIND_PACKAGE (SWIG)
            IF (SWIG_FOUND)
                MESSAGE (STATUS "Python bindings for C++ client library will be built")

                #INCLUDE (${SWIG_USE_FILE})
                INCLUDE (${PLAYER_CMAKE_DIR}/internal/UseSwig_Player.cmake)

                FIND_PACKAGE (PythonLibs)
                INCLUDE_DIRECTORIES (${PYTHON_INCLUDE_PATH})
                INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR}/client_libs)
                INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

                # Use playercpp.i from source directory
                SET (playercpp_i "${CMAKE_CURRENT_SOURCE_DIR}/playercpp.i")

                SET_SOURCE_FILES_PROPERTIES(${playercpp_i} PROPERTIES CPLUSPLUS ON)

                SET (SWIG_MODULE_playercpp_EXTRA_DEPS ${PROJECT_SOURCE_DIR}/client_libs/libplayerc++/playerc++.h)
                SWIG_ADD_MODULE (playercpp python ${playercpp_i})
                SWIG_LINK_LIBRARIES (playercpp ${PYTHON_LIBRARIES} playerc++)

                IF (PYTHON_OS_WIN)
                    GET_FILENAME_COMPONENT (playercpyInstallDir ${PYTHON_EXECUTABLE} PATH)
                ELSE (PYTHON_OS_WIN)
                    # Get the Python version
                    EXECUTE_PROCESS (COMMAND ${PYTHON_EXECUTABLE} -V
                                    ERROR_VARIABLE pythonVersionString
                                    ERROR_STRIP_TRAILING_WHITESPACE)
                    STRING (REGEX REPLACE "^Python ([0-9]+\\.[0-9]+).*" "\\1" pythonVersion ${pythonVersionString})
                    SET (playercpppyInstallDir ${PLAYER_LIBRARY_INSTALL_DIR}/python${pythonVersion}/site-packages)
                ENDIF (PYTHON_OS_WIN)
                SET (PYTHON_BINDINGS_INSTALL_DIR ${playercpppyInstallDir} CACHE PATH "Python bindings installation directory under $prefix")
                MARK_AS_ADVANCED (PYTHON_BINDINGS_INSTALL_DIR)

                INSTALL (FILES ${CMAKE_CURRENT_BINARY_DIR}/playercpp.py
                        ${CMAKE_CURRENT_BINARY_DIR}/_playercpp.so
                        DESTINATION ${PYTHON_BINDINGS_INSTALL_DIR})
            ELSE (SWIG_FOUND)
                MESSAGE (STATUS "Python bindings for C++ client library will not be built - could not find Swig")
            ENDIF (SWIG_FOUND)
        ELSE (BUILD_PYTHONCPP_BINDINGS)
            MESSAGE (STATUS "Python bindings for C++ client library will not be built - disabled by user")
        ENDIF (BUILD_PYTHONCPP_BINDINGS)
    ELSE (PYTHONINTERP_FOUND)
        MESSAGE (STATUS "Python bindings for C++ client library will not be built - could not find Python")
    ENDIF (PYTHONINTERP_FOUND)
ELSE (BUILD_PLAYERCC)
  MESSAGE (STATUS "Python bindings for C++ client library will not be built - playerc++ client library disabled")
ENDIF (BUILD_PLAYERCC)
