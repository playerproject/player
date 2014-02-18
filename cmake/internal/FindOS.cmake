# Check the OS type.

# CMake does not distinguish Linux from other Unices.
STRING (REGEX MATCH "Linux" PLAYER_OS_LINUX ${CMAKE_SYSTEM_NAME})
# Nor *BSD
STRING (REGEX MATCH "BSD" PLAYER_OS_BSD ${CMAKE_SYSTEM_NAME})
# Or Solaris. I'm seeing a trend, here
STRING (REGEX MATCH "SunOS" PLAYER_OS_SOLARIS ${CMAKE_SYSTEM_NAME})

# Export CMAKE's UNIX variable to source files
IF(UNIX)
  ADD_DEFINITIONS(-DPLAYER_UNIX)
ENDIF(UNIX)

# Windows is easy (for once)
IF (WIN32)
    SET (PLAYER_OS_WIN TRUE BOOL INTERNAL)
ENDIF (WIN32)

# Check if it's an Apple OS
IF (APPLE)
    # Check if it's OS X or another MacOS (that's got to be pretty unlikely)
    STRING (REGEX MATCH "Darwin" PLAYER_OS_OSX ${CMAKE_SYSTEM_NAME})
    IF (NOT PLAYER_OS_OSX)
        SET (PLAYER_OS_MACOS TRUE BOOL INTERNAL)
    ENDIF (NOT PLAYER_OS_OSX)
ENDIF (APPLE)

# QNX
IF (QNXNTO)
    SET (PLAYER_OS_QNX TRUE BOOL INTERNAL)
ENDIF (QNXNTO)

IF (PLAYER_OS_LINUX)
    MESSAGE (STATUS "Operating system is Linux")
ELSEIF (PLAYER_OS_BSD)
    MESSAGE (STATUS "Operating system is BSD")
ELSEIF (PLAYER_OS_WIN)
    MESSAGE (STATUS "Operating system is Windows")
ELSEIF (PLAYER_OS_OSX)
    MESSAGE (STATUS "Operating system is Apple MacOS X")
ELSEIF (PLAYER_OS_MACOS)
    MESSAGE (STATUS "Operating system is Apple MacOS (not OS X)")
ELSEIF (PLAYER_OS_QNX)
    MESSAGE (STATUS "Operating system is QNX")
ELSEIF (PLAYER_OS_SOLARIS)
    MESSAGE (STATUS "Operating system is Solaris")
ELSE (PLAYER_OS_LINUX)
    MESSAGE (STATUS "Operating system is generic Unix")
ENDIF (PLAYER_OS_LINUX)


MESSAGE (STATUS "Got System Processor ${CMAKE_SYSTEM_PROCESSOR}")

# 32 or 64 bit Linux
IF (PLAYER_OS_LINUX)
    # Set the library directory suffix accordingly
    IF (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
        SET (PLAYER_PROC_64BIT TRUE BOOL INTERNAL)
        MESSAGE (STATUS "Linux x86_64 Detected")
    ELSEIF (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "ppc64")
        MESSAGE (STATUS "Linux ppc64 Detected")
        SET (PLAYER_PROC_64BIT TRUE BOOL INTERNAL)
    ENDIF (${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
ENDIF (PLAYER_OS_LINUX)

