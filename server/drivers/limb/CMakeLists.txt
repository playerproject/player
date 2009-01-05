PLAYERDRIVER_OPTION (eedhcontroller build_eedhcontroller OFF "Disabled - probably doesn't build")
PLAYERDRIVER_REQUIRE_LIB (eedhcontroller build_eedhcontroller roboop main "")
PLAYERDRIVER_REQUIRE_LIB (eedhcontroller build_eedhcontroller newmat main "")
PLAYERDRIVER_ADD_DRIVER (eedhcontroller build_eedhcontroller SOURCES eeDHcontroller.cc)
