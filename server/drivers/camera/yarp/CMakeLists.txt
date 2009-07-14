PLAYERDRIVER_OPTION (yarpimage build_yarpimage ON)
PLAYERDRIVER_REQUIRE_HEADER (yarpimage build_yarpimage yarp/os/all.h)
PLAYERDRIVER_REQUIRE_HEADER (yarpimage build_yarpimage yarp/sig/all.h)
PLAYERDRIVER_ADD_DRIVER (yarpimage build_yarpimage LINKFLAGS -lYARP_sig -lYARP_OS -lACE SOURCES YarpImage.cc)
