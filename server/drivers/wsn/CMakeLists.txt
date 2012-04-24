IF (HAVE_STL)
    PLAYERDRIVER_OPTION (mica2 build_mica2 ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (mica2 build_mica2 OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_REJECT_OS (mica2 build_mica2 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (mica2 build_mica2 SOURCES mica2.cc)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (rcore_xbridge build_rcore_xbridge ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (rcore_xbridge build_rcore_xbridge OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_REQUIRE_HEADER (rcore_xbridge build_rcore_xbridge libparticle.h)
PLAYERDRIVER_ADD_DRIVER (rcore_xbridge build_rcore_xbridge LINKFLAGS "-lparticle" SOURCES rcore_xbridge.cc)

IF (HAVE_STL)
    PLAYERDRIVER_OPTION (accel_calib build_accel_calib ON)
ELSE (HAVE_STL)
    PLAYERDRIVER_OPTION (accel_calib build_accel_calib OFF "STL not found.")
ENDIF (HAVE_STL)
PLAYERDRIVER_ADD_DRIVER (accel_calib build_accel_calib SOURCES accel_calib.cc)

# libphidget now discovered in SearchForStuff.cmake
IF (HAVE_PHIDGET)
  PLAYERDRIVER_OPTION (phidgetAcc build_phidgetAcc ON)
ELSE (HAVE_PHIDGET)
  PLAYERDRIVER_OPTION (phidgetAcc build_phidgetAcc OFF "Could not find libphidget")
ENDIF (HAVE_PHIDGET)
PLAYERDRIVER_ADD_DRIVER (phidgetAcc build_phidgetAcc
    LINKFLAGS ${phidgetLinkFlags} CFLAGS ${phidgetCFlags}
    SOURCES phidgetAcc.cc)


IF(HAVE_LIBSERIAL)
  add_subdirectory(mote)

  IF (HAVE_STL)
      PLAYERDRIVER_OPTION (generic_xbow build_generic_xbow ON)
  ELSE (HAVE_STL)
      PLAYERDRIVER_OPTION (generic_xbow build_generic_xbow OFF "STL not found.")
  ENDIF (HAVE_STL)
  PLAYERDRIVER_REQUIRE_HEADER(generic_xbow build_generic_xbow SerialStream.h)
  PLAYERDRIVER_ADD_DRIVER (generic_xbow build_generic_xbow LINKFLAGS "-L${CMAKE_CURRENT_BINARY_DIR}/mote" "-lmote" "-lserial" SOURCES generic_xbow.cc generic_xbow_structdef.cc)
ENDIF(HAVE_LIBSERIAL)
