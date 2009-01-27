PLAYERDRIVER_OPTION (aodv build_aodv ON)
PLAYERDRIVER_REJECT_OS (aodv build_aodv PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (aodv build_aodv SOURCES aodv.cc)

PLAYERDRIVER_OPTION (iwspy build_iwspy ON)
PLAYERDRIVER_REJECT_OS (iwspy build_iwspy PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (iwspy build_iwspy SOURCES iwspy.cc)

PLAYERDRIVER_OPTION (linuxwifi build_linuxwifi ON)
PLAYERDRIVER_REJECT_OS (linuxwifi build_linuxwifi PLAYER_OS_WIN)
PLAYERDRIVER_REQUIRE_HEADER (linuxwifi build_linuxwifi linux/wireless.h)
PLAYERDRIVER_ADD_DRIVER (linuxwifi build_linuxwifi SOURCES linuxwifi.cc)
