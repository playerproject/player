PLAYERDRIVER_OPTION (mica2 build_mica2 ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (mica2 build_mica2 mica2.cc)

PLAYERDRIVER_OPTION (rcore_xbridge build_rcore_xbridge ON)
PLAYERDRIVER_REQUIRE_HEADER (rcore_xbridge build_rcore_xbridge libparticle.h)
PLAYERDRIVER_ADD_DRIVER (rcore_xbridge build_rcore_xbridge "" "" "-lparticle" "" rcore_xbridge.cc)

PLAYERDRIVER_OPTION (accel_calib build_accel_calib ON)
PLAYERDRIVER_ADD_DRIVER_SIMPLE (accel_calib build_accel_calib accel_calib.cc)