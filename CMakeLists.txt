IF (WIN32)
    # Require 2.6 for Windows
    CMAKE_MINIMUM_REQUIRED (VERSION 2.6 FATAL_ERROR)
ELSE (WIN32)
    CMAKE_MINIMUM_REQUIRED (VERSION 2.4.7 FATAL_ERROR)
ENDIF (WIN32)

# Compatibility settings
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY (SET CMP0003 NEW)
    CMAKE_POLICY (SET CMP0004 NEW)
ENDIF (COMMAND CMAKE_POLICY)


# Set the project name (helps Visual Studio, mainly)
PROJECT (Player)
STRING (TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
# Set the package version
SET (PLAYER_VERSION 3.0.2 CACHE STRING "Player distribution version")
SET (PLAYER_API_VERSION 3.0 CACHE STRING "Player API version")
MESSAGE (STATUS "${PROJECT_NAME} version ${PLAYER_VERSION}")

# Set where to find our internal CMake scripts
SET (PLAYER_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake CACHE PATH "Location of CMake scripts")
# Get version components
INCLUDE (${PLAYER_CMAKE_DIR}/internal/DissectVersion.cmake)
# Determine the operating system in detail
INCLUDE (${PLAYER_CMAKE_DIR}/internal/FindOS.cmake)

# Enable -Wall by default
IF (NOT PLAYER_OS_WIN AND NOT PLAYER_OS_SOLARIS)
    # Using -Wall on Windows causes MSVC to produce thousands of warnings in its
    # own standard headers, dramatically slowing down the build.
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
ENDIF (NOT PLAYER_OS_WIN AND NOT PLAYER_OS_SOLARIS)

# Check the compiler version is OK
# INCLUDE (${PLAYER_CMAKE_DIR}/internal/CheckCompiler.cmake)
# Setup directories such as install destination
INCLUDE (${PLAYER_CMAKE_DIR}/internal/SetupDirectories.cmake)
# Uninstall target
INCLUDE (${PLAYER_CMAKE_DIR}/internal/UninstallTarget.cmake)

# Some options to control the build
OPTION (PLAYER_BUILD_TESTS "Enables compilation of the test suites" ON)

# Look for various needed things
INCLUDE (${PLAYER_CMAKE_DIR}/internal/SearchForStuff.cmake)
# Give the user some compile options
INCLUDE (${PLAYER_CMAKE_DIR}/internal/GeneralCompileOptions.cmake)
# Write the config.h file
CONFIGURE_FILE (${PROJECT_SOURCE_DIR}/config.h.in ${PROJECT_BINARY_DIR}/config.h)
# Generate playerconfig.h
SET (playerconfig_h_in "${CMAKE_CURRENT_SOURCE_DIR}/playerconfig.h.in")
SET (playerconfig_h "${CMAKE_CURRENT_BINARY_DIR}/playerconfig.h")
CONFIGURE_FILE (${playerconfig_h_in} ${playerconfig_h})
INSTALL (FILES ${playerconfig_h} DESTINATION ${PLAYER_INCLUDE_INSTALL_DIR} COMPONENT headers)
# Include some useful macros
INCLUDE (${PLAYER_CMAKE_DIR}/internal/LibraryUtils.cmake)

# Set some common include directories, including the binary dir to get config.h and playerconfig.h
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${PROJECT_BINARY_DIR}/libplayercore)
# Extra directories where needed libraries, etc may be located (set by the user)
INCLUDE_DIRECTORIES (${PLAYER_EXTRA_INCLUDE_DIRS})
LINK_DIRECTORIES (${PLAYER_EXTRA_LIB_DIRS})

IF (PLAYER_OS_WIN)
    # This is pretty much a universal include on Windows
    INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/replace)
ENDIF (PLAYER_OS_WIN)

ADD_SUBDIRECTORY (libplayercommon)
ADD_SUBDIRECTORY (libplayerinterface)
ADD_SUBDIRECTORY (client_libs)
ADD_SUBDIRECTORY (libplayercore)
ADD_SUBDIRECTORY (config)           # Example config files
ADD_SUBDIRECTORY (libplayerwkb)
ADD_SUBDIRECTORY (libplayerjpeg)
ADD_SUBDIRECTORY (libplayertcp)
ADD_SUBDIRECTORY (libplayersd)
ADD_SUBDIRECTORY (rtk2)
ADD_SUBDIRECTORY (server)
ADD_SUBDIRECTORY (examples)
ADD_SUBDIRECTORY (utils)
ADD_SUBDIRECTORY (doc)
ADD_SUBDIRECTORY (cmake)            # CMake modules for Player libraries
ADD_SUBDIRECTORY (replace)
MESSAGE (STATUS "")

PLAYER_CLEAR_CACHED_LISTS ()

# Create packages
INCLUDE (InstallRequiredSystemLibraries)
CONFIGURE_FILE ("${PROJECT_SOURCE_DIR}/PlayerCPackOptions.cmake.in" "${PROJECT_BINARY_DIR}/PlayerCPackOptions.cmake" @ONLY)
SET (CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/PlayerCPackOptions.cmake")
INCLUDE (CPack)
