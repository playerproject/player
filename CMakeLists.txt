CMAKE_MINIMUM_REQUIRED (VERSION 2.4.7 FATAL_ERROR)

# Compatibility settings
IF (COMMAND CMAKE_POLICY)
    CMAKE_POLICY (SET CMP0003 NEW)
    CMAKE_POLICY (SET CMP0004 NEW)
ENDIF (COMMAND CMAKE_POLICY)


# Set the project name (helps Visual Studio, mainly)
PROJECT (Player)
STRING (TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)
# Set the package version
SET (PLAYER_VERSION 2.2.0 CACHE STRING "Player distribution version")
SET (PLAYER_API_VERSION 2.2 CACHE STRING "Player API version")
MESSAGE (STATUS "${PROJECT_NAME} version ${PLAYER_VERSION}")

# Set where to find our internal CMake scripts
SET (PLAYER_CMAKE_DIR ${PROJECT_SOURCE_DIR}/cmake CACHE PATH "Location of CMake scripts")
# Get version components
INCLUDE (${PLAYER_CMAKE_DIR}/internal/DissectVersion.cmake)
# Determine the operating system in detail
INCLUDE (${PLAYER_CMAKE_DIR}/internal/FindOS.cmake)
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
INSTALL (FILES ${playerconfig_h} DESTINATION ${PLAYER_INCLUDE_INSTALL_DIR})
# Include some useful macros
INCLUDE (${PLAYER_CMAKE_DIR}/internal/LibraryUtils.cmake)

# Set some common include directories, including the binary dir to get config.h
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${PROJECT_BINARY_DIR}/libplayercore)
# Extra directories where needed libraries, etc may be located (set by the user)
INCLUDE_DIRECTORIES (${PLAYER_EXTRA_INCLUDE_DIRS})
LINK_DIRECTORIES (${PLAYER_EXTRA_LIB_DIRS})

ADD_SUBDIRECTORY (client_libs)
ADD_SUBDIRECTORY (libplayercore)
ADD_SUBDIRECTORY (libplayerxdr)
ADD_SUBDIRECTORY (config)           # Example config files
ADD_SUBDIRECTORY (libplayerjpeg)
ADD_SUBDIRECTORY (libplayertcp)
ADD_SUBDIRECTORY (libplayersd)
ADD_SUBDIRECTORY (rtk2)
ADD_SUBDIRECTORY (server)
ADD_SUBDIRECTORY (examples)
IF (NOT PLAYER_OS_WIN)
    ADD_SUBDIRECTORY (utils)
    ADD_SUBDIRECTORY (doc)
ENDIF (NOT PLAYER_OS_WIN)
ADD_SUBDIRECTORY (cmake)            # CMake modules for Player libraries
IF (PLAYER_OS_WIN)
    ADD_SUBDIRECTORY (replace)
ENDIF (PLAYER_OS_WIN)
MESSAGE (STATUS "")

SET (PLAYER_EXTRA_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link to" FORCE)
SET (PLAYERC_EXTRA_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link to with playerc" FORCE)
SET (PLAYERCC_EXTRA_LINK_LIBRARIES "" CACHE INTERNAL "Libs to link to with playercc" FORCE)

# Create packages
INCLUDE (InstallRequiredSystemLibraries)
SET (CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYING")
SET (CPACK_PACKAGE_VERSION_MAJOR "${PLAYER_MAJOR_VERSION}")
SET (CPACK_PACKAGE_VERSION_MINOR "${PLAYER_MINOR_VERSION}")
SET (CPACK_PACKAGE_VERSION_PATCH "${PLAYER_REVISION_VERSION}${PLAYER_CANDIDATE_VERSION}")
INCLUDE (CPack)
