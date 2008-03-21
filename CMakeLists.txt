CMAKE_MINIMUM_REQUIRED (VERSION 2.4 FATAL_ERROR)

# Set the project name (helps Visual Studio, mainly)
PROJECT (Player)
# Set the package version
SET (PLAYER_VERSION 2.1.0 CACHE STRING "Player distribution version")
SET (PLAYER_API_VERSION 2.1.0 CACHE STRING "Player API version")
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
OPTION (PLAYER_BUILT_TESTS "Enables compilation of the test suites" ON)

# Look for various needed things
INCLUDE (${PLAYER_CMAKE_DIR}/internal/SearchForStuff.cmake)
# Give the user some compile options
INCLUDE (${PLAYER_CMAKE_DIR}/internal/GeneralCompileOptions.cmake)
# Write the config.h file
CONFIGURE_FILE (${PROJECT_SOURCE_DIR}/config.h.cmake ${PROJECT_BINARY_DIR}/config.h)

# Include some useful macros
INCLUDE (${PLAYER_CMAKE_DIR}/internal/LibraryUtils.cmake)

# Set some common include directories, including the binary dir to get config.h
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${PROJECT_BINARY_DIR}/libplayercore)

# If on Windows, only build the client libraries. (this won't actually work yet cause libplayerc needs some stuff from core)
ADD_SUBDIRECTORY (client_libs)
IF (NOT PLAYER_OS_WIN)
    ADD_SUBDIRECTORY (libplayercore)
    ADD_SUBDIRECTORY (config)           # Example config files
    ADD_SUBDIRECTORY (libplayerxdr)
    ADD_SUBDIRECTORY (libplayerjpeg)
    ADD_SUBDIRECTORY (libplayertcp)
    ADD_SUBDIRECTORY (libplayersd)
    ADD_SUBDIRECTORY (rtk2)
    ADD_SUBDIRECTORY (replace)
    ADD_SUBDIRECTORY (server)
    ADD_SUBDIRECTORY (examples)
    ADD_SUBDIRECTORY (utils)
#     ADD_SUBDIRECTORY (doc)
ENDIF (NOT PLAYER_OS_WIN)
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