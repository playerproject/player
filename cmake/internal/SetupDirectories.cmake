# Default installation directory, based on operating system
IF (PLAYER_OS_WIN)
    SET (CMAKE_INSTALL_PREFIX "C:\\Program Files\\Player" CACHE PATH "Installation directory")
ELSE (PLAYER_OS_WIN)
    SET (CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation directory")
ENDIF (PLAYER_OS_WIN)

MESSAGE (STATUS "Player will be installed to ${CMAKE_INSTALL_PREFIX}")

# Installation prefix for include files
STRING (TOLOWER ${PROJECT_NAME} projectNameLower)
SET (PLAYER_INCLUDE_INSTALL_DIR "include/${projectNameLower}-${PLAYER_MAJOR_VERSION}.${PLAYER_MINOR_VERSION}")

IF (PLAYER_PROC_64BIT)
    SET (LIB_SUFFIX "64" CACHE STRING "Suffix for library installation directory")
ELSE (PLAYER_PROC_64BIT)
    SET (LIB_SUFFIX "" CACHE STRING "Suffix for library installation directory")
ENDIF (PLAYER_PROC_64BIT)

SET (PLAYER_LIBRARY_INSTALL_DIR "lib${LIB_SUFFIX}")

MESSAGE (STATUS "Headers will be installed to ${CMAKE_INSTALL_PREFIX}/${PLAYER_INCLUDE_INSTALL_DIR}")
MESSAGE (STATUS "Libraries will be installed to ${CMAKE_INSTALL_PREFIX}/${PLAYER_LIBRARY_INSTALL_DIR}")

