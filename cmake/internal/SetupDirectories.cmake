# Default installation directory, based on operating system
IF (PLAYER_OS_WIN)
    SET (CMAKE_INSTALL_PREFIX "C:\Program Files\Player" CACHE PATH "Installation directory")
ELSE (PLAYER_OS_WIN)
    SET (CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation directory")
ENDIF (PLAYER_OS_WIN)

MESSAGE (STATUS "Player will be installed to ${CMAKE_INSTALL_PREFIX}")