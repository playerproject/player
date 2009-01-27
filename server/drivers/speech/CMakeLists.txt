ADD_SUBDIRECTORY (recognition)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (festival build_festival ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (festival build_festival OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_REJECT_OS (festival build_festival PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (festival build_festival SOURCES festival.cc)
