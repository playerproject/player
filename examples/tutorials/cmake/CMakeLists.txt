# CMake Example Project

# Require
IF (WIN32)
    # Require 2.6 for Windows
    CMAKE_MINIMUM_REQUIRED (VERSION 2.6 FATAL_ERROR)
ELSE (WIN32)
    CMAKE_MINIMUM_REQUIRED (VERSION 2.4.7 FATAL_ERROR)
ENDIF (WIN32)

# Set your project name and version
PROJECT (CMakeExampleProject)
SET(PROJECT_VERSION 0.0.1 CACHE STRING "CMake Example Project Version Number")

# Build flags for different configurations
# Use is determined by the option CMAKE_BUILD_TYPE
set(CMAKE_C_FLAGS_DEBUG "-g -Wall ")
set(CMAKE_C_FLAGS_RELEASE "-O2 ")

set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall ")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 ")

# Add an uninstall target (for "make uninstall")
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)
ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

# Add subdirectories so those CMakeLists.txt files are parsed  
ADD_SUBDIRECTORY (src)
ADD_SUBDIRECTORY (doc)
