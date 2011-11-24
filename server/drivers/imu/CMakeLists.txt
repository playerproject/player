ADD_SUBDIRECTORY(xsens)

PLAYERDRIVER_OPTION (nimu build_nimu ON)
PLAYERDRIVER_REQUIRE_HEADER (nimu build_nimu usb.h)
PLAYERDRIVER_REJECT_OS (nimu build_nimu PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (nimu build_nimu LINKFLAGS "-lusb" SOURCES nimu.cpp nimuplayer.cpp)

# razorimu requires termios.h, reject Windows
PLAYERDRIVER_OPTION (razorimu build_razorimu ON)
PLAYERDRIVER_REJECT_OS (razorimu build_razorimu PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (razorimu build_razorimu SOURCES RazorIMU.cc)
