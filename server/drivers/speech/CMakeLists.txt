ADD_SUBDIRECTORY (recognition)

PLAYERDRIVER_OPTION (festival build_festival ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (festival build_festival festival.cc)