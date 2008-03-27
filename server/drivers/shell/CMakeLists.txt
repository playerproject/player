PLAYERDRIVER_OPTION (dummy build_dummy ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (dummy build_dummy dummy.cc)

PLAYERDRIVER_OPTION (kartowriter build_kartowriter ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (kartowriter build_kartowriter kartowriter.cc)

PLAYERDRIVER_OPTION (writelog build_writelog ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (writelog build_writelog writelog.cc encode.cc)

PLAYERDRIVER_OPTION (readlog build_readlog ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (readlog build_readlog encode.cc readlog_time.cc readlog.cc)

PLAYERDRIVER_OPTION (passthrough build_passthrough ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (passthrough build_passthrough passthrough.cc)

PLAYERDRIVER_OPTION (relay build_relay ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (relay build_relay relay.cc)