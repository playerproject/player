PLAYERDRIVER_OPTION (XSensMT build_XSensMT ON)
PLAYERDRIVER_ADD_DRIVER (XSensMT build_XSensMT SOURCES XSensMT.cc MTComm.cpp)

PLAYERDRIVER_OPTION (nimu build_nimu ON)
PLAYERDRIVER_REQUIRE_HEADER (nimu build_nimu usb.h)
PLAYERDRIVER_ADD_DRIVER (nimu build_nimu LINKFLAGS "-lusb" SOURCES nimu.cpp nimuplayer.cpp)