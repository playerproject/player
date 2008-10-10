PLAYERDRIVER_OPTION (stoc build_stoc ON)
PLAYERDRIVER_ADD_DRIVER (stoc build_stoc LINKFLAGS "-lsvscap -lsvs -lsvscalc -lpthread -ldcap -lraw1394" SOURCES stoc.cc)
