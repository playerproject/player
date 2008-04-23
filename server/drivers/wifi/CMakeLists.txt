PLAYERDRIVER_OPTION (aodv build_aodv ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (aodv build_aodv aodv.cc)

PLAYERDRIVER_OPTION (iwspy build_iwspy ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (iwspy build_iwspy iwspy.cc)

PLAYERDRIVER_OPTION (linuxwifi build_linuxwifi ON)
PLAYERDRIVER_REQUIRE_HEADER (linuxwifi build_linuxwifi linux/wireless.h)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (linuxwifi build_linuxwifi linuxwifi.cc)