PLAYERDRIVER_OPTION (XSensMT build_XSensMT ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (XSensMT build_XSensMT XSensMT.cc MTComm.cpp)

PLAYERDRIVER_OPTION (nimu build_nimu ON)
PLAYERDRIVER_REQUIRE_HEADER (nimu build_nimu usb.h)
PLAYERDRIVER_ADD_DRIVER (nimu build_nimu "" "" "-lusb" "" nimu.cpp nimuplayer.cpp)