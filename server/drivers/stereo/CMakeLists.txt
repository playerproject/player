PLAYERDRIVER_OPTION (stoc build_stoc ON)
PLAYERDRIVER_REQUIRE_HEADER_CPP (stoc build_stoc SVS/svsclass.h)
PLAYERDRIVER_ADD_DRIVER (stoc build_stoc LINKFLAGS "-lsvscap -lsvs -lsvscalc -lpthread -ldcap -lraw1394" SOURCES stoc.cc)
