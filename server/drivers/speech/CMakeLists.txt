ADD_SUBDIRECTORY (recognition)

PLAYERDRIVER_OPTION (festival build_festival ON)
PLAYERDRIVER_ADD_DRIVER (festival build_festival SOURCES festival.cc)