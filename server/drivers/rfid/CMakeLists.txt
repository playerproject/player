PLAYERDRIVER_OPTION (insideM300 build_insideM300 ON)
PLAYERDRIVER_REJECT_OS (insideM300 build_insideM300 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (insideM300 build_insideM300 SOURCES insideM300.cc)

PLAYERDRIVER_OPTION (sickRFI341 build_sickRFI341 ON)
PLAYERDRIVER_REJECT_OS (sickRFI341 build_sickRFI341 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (sickRFI341 build_sickRFI341 SOURCES rfi341_protocol.cc sickrfi341.cc)

PLAYERDRIVER_OPTION (skyetekM1 build_skyetekM1 ON)
PLAYERDRIVER_REJECT_OS (skyetekM1 build_skyetekM1 PLAYER_OS_WIN)
PLAYERDRIVER_ADD_DRIVER (skyetekM1 build_skyetekM1 SOURCES skyetekM1.cc)

PLAYERDRIVER_OPTION (phidgetRFID build_phidgetRFID ON)
SET (PHIDGETRFID_DIR "" CACHE STRING "Directory containing the Phidget RFID headers and libraries")
MARK_AS_ADVANCED (PHIDGETRFID_DIR)
IF ("${PHIDGETRFID_DIR}" STREQUAL "")
    SET (phidgetReqHeader "phidget21.h")
    SET (phidgetExtraFlags "")
    SET (phidgetExtraLibs "-lphidget21")
ELSE ("${PHIDGETRFID_DIR}" STREQUAL "")
    SET (phidgetReqHeader "${PHIDGETRFID_DIR}/phidget21.h")
    SET (phidgetExtraFlags "-I${PHIDGETRFID_DIR}/include")
    SET (phidgetExtraLibs "-L${PHIDGETRFID_DIR}/lib -lphidget21")
ENDIF ("${PHIDGETRFID_DIR}" STREQUAL "")
PLAYERDRIVER_REQUIRE_HEADER (phidgetRFID build_phidgetRFID ${phidgetReqHeader})
PLAYERDRIVER_ADD_DRIVER (phidgetRFID build_phidgetRFID
    LINKFLAGS ${phidgetExtraLibs} CFLAGS "${phidgetExtraFlags}"
    SOURCES phidgetRFID.cc)

PLAYERDRIVER_OPTION (acr120u build_acr120u ON)
PLAYERDRIVER_REQUIRE_HEADER (acr120u build_acr120u "usb.h")
PLAYERDRIVER_ADD_DRIVER (acr120u build_acr120u LINKFLAGS "-lusb" SOURCES acr120u.cc)
