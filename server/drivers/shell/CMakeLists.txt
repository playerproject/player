PLAYERDRIVER_OPTION (dummy build_dummy ON)
PLAYERDRIVER_ADD_DRIVER (dummy build_dummy SOURCES dummy.cc)

PLAYERDRIVER_OPTION (kartowriter build_kartowriter ON)
PLAYERDRIVER_ADD_DRIVER (kartowriter build_kartowriter SOURCES kartowriter.cc)

PLAYERDRIVER_OPTION (writelog build_writelog ON)
PLAYERDRIVER_ADD_DRIVER (writelog build_writelog SOURCES writelog.cc encode.cc)

PLAYERDRIVER_OPTION (readlog build_readlog ON)
IF (HAVE_Z)
    SET (readlogLinkFlags -lz)
ENDIF (HAVE_Z)
PLAYERDRIVER_ADD_DRIVER (readlog build_readlog SOURCES encode.cc readlog_time.cc readlog.cc
                        LINKFLAGS ${readlogLinkFlags})

PLAYERDRIVER_OPTION (passthrough build_passthrough ON)
PLAYERDRIVER_ADD_DRIVER (passthrough build_passthrough SOURCES passthrough.cc)

PLAYERDRIVER_OPTION (relay build_relay ON)
PLAYERDRIVER_ADD_DRIVER (relay build_relay SOURCES relay.cc)
