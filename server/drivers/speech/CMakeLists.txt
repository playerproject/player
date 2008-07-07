ADD_SUBDIRECTORY (recognition)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (festival build_festival ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (festival build_festival OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_ADD_DRIVER (festival build_festival SOURCES festival.cc)
