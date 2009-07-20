ADD_SUBDIRECTORY(xsens)

PLAYERDRIVER_OPTION (nimu build_nimu ON)
PLAYERDRIVER_REQUIRE_HEADER (nimu build_nimu usb.h)
PLAYERDRIVER_REJECT_OS (nimu build_nimu PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (nimu build_nimu LINKFLAGS "-lusb" SOURCES nimu.cpp nimuplayer.cpp)

