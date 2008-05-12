PLAYERDRIVER_OPTION (mica2 build_mica2 ON)
PLAYERDRIVER_ADD_DRIVER (mica2 build_mica2 SOURCES mica2.cc)

PLAYERDRIVER_OPTION (rcore_xbridge build_rcore_xbridge ON)
PLAYERDRIVER_REQUIRE_HEADER (rcore_xbridge build_rcore_xbridge libparticle.h)
PLAYERDRIVER_ADD_DRIVER (rcore_xbridge build_rcore_xbridge LINKFLAGS "-lparticle" SOURCES rcore_xbridge.cc)

PLAYERDRIVER_OPTION (accel_calib build_accel_calib ON)
PLAYERDRIVER_ADD_DRIVER (accel_calib build_accel_calib SOURCES accel_calib.cc)
