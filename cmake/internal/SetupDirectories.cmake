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
