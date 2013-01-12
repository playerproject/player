# Default installation directory, based on operating system
IF (PLAYER_OS_WIN)
    SET (CMAKE_INSTALL_PREFIX "C:\\Program Files\\Player" CACHE PATH "Installation prefix")
ELSE (PLAYER_OS_WIN)
    SET (CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix")
ENDIF (PLAYER_OS_WIN)

MESSAGE (STATUS "Player will be installed to ${CMAKE_INSTALL_PREFIX}")

# Installation prefix for include files
STRING (TOLOWER ${PROJECT_NAME} projectNameLower)
SET (PLAYER_INCLUDE_INSTALL_DIR "include/${projectNameLower}-${PLAYER_MAJOR_VERSION}.${PLAYER_MINOR_VERSION}")

# Let the user take care of this
SET(LIB_SUFFIX "" CACHE STRING "Suffix for library installation directory")
#IF (PLAYER_PROC_64BIT)
#    SET (LIB_SUFFIX "64" CACHE STRING "Suffix for library installation directory")
#ELSE (PLAYER_PROC_64BIT)
#    SET (LIB_SUFFIX "" CACHE STRING "Suffix for library installation directory")
#ENDIF (PLAYER_PROC_64BIT)

SET (PLAYER_LIBRARY_INSTALL_DIR "lib${LIB_SUFFIX}")
SET (PLAYER_PLUGIN_INSTALL_DIR "${PLAYER_LIBRARY_INSTALL_DIR}/${projectNameLower}-${PLAYER_MAJOR_VERSION}.${PLAYER_MINOR_VERSION}")

MESSAGE (STATUS "Headers will be installed to ${CMAKE_INSTALL_PREFIX}/${PLAYER_INCLUDE_INSTALL_DIR}")
MESSAGE (STATUS "Libraries will be installed to ${CMAKE_INSTALL_PREFIX}/${PLAYER_LIBRARY_INSTALL_DIR}")
MESSAGE (STATUS "Plugins will be installed to ${CMAKE_INSTALL_PREFIX}/${PLAYER_PLUGIN_INSTALL_DIR}")
