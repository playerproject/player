# libphidget now discovered in SearchForStuff.cmake
IF (HAVE_PHIDGET)
  PLAYERDRIVER_OPTION (phidgetifk build_phidgetifk ON)
ELSE (HAVE_PHIDGET)
  PLAYERDRIVER_OPTION (phidgetifk build_phidgetifk OFF "Could not find libphidget")
ENDIF (HAVE_PHIDGET)
  
PLAYERDRIVER_ADD_DRIVER (phidgetifk build_phidgetifk
    LINKFLAGS ${phidgetLinkFlags} CFLAGS ${phidgetCFlags}
    SOURCES phidgetIFK.cc)
