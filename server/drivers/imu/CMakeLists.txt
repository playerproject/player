ADD_SUBDIRECTORY(xsens)

PLAYERDRIVER_OPTION (nimu build_nimu ON)
PLAYERDRIVER_REQUIRE_HEADER (nimu build_nimu usb.h)
PLAYERDRIVER_ADD_DRIVER (nimu build_nimu LINKFLAGS "-lusb" SOURCES nimu.cpp nimuplayer.cpp)

PLAYERDRIVER_OPTION (rt3xxx build_re3xxx ON)
PLAYERDRIVER_ADD_DRIVER (rt3xxx build_rt3xxx SOURCES rt3xxx.cc)
