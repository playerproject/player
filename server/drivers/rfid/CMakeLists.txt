PLAYERDRIVER_OPTION (insideM300 build_insideM300 ON)
PLAYERDRIVER_REJECT_OS (insideM300 build_insideM300 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (insideM300 build_insideM300 SOURCES insideM300.cc)

PLAYERDRIVER_OPTION (sickRFI341 build_sickRFI341 ON)
PLAYERDRIVER_REJECT_OS (sickRFI341 build_sickRFI341 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (sickRFI341 build_sickRFI341 SOURCES rfi341_protocol.cc sickrfi341.cc)

PLAYERDRIVER_OPTION (skyetekM1 build_skyetekM1 ON)
PLAYERDRIVER_REJECT_OS (skyetekM1 build_skyetekM1 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (skyetekM1 build_skyetekM1 SOURCES skyetekM1.cc)

# libphidget now discovered in SearchForStuff.cmake
IF (HAVE_PHIDGET)
  PLAYERDRIVER_OPTION (phidgetAcc build_phidgetRFID ON)
ELSE (HAVE_PHIDGET)
  PLAYERDRIVER_OPTION (phidgetAcc build_phidgetRFID OFF "Could not find libphidget")
ENDIF (HAVE_PHIDGET)

PLAYERDRIVER_ADD_DRIVER (phidgetRFID build_phidgetRFID
    LINKFLAGS ${phidgetLinkFlags} CFLAGS "${phidgetCFlags}"
    SOURCES phidgetRFID.cc)

PLAYERDRIVER_OPTION (acr120u build_acr120u ON)
PLAYERDRIVER_REQUIRE_HEADER (acr120u build_acr120u "usb.h")
PLAYERDRIVER_ADD_DRIVER (acr120u build_acr120u LINKFLAGS "-lusb" SOURCES acr120u.cc)
