PLAYERDRIVER_OPTION (segwayrmp build_segwayrmp ON "Build Segway RMP driver")
PLAYERDRIVER_REJECT_OS(segwayrmp build_segwayrmp PLAYER_OS_WIN)
SET (CANLIB_DIR "" CACHE STRING "Directory containing the CANLIB headers and libraries")

IF (${HAVE_CANLIB})
PLAYERDRIVER_ADD_DRIVER (segwayrmp build_segwayrmp
    LINKFLAGS ${CANLIB_LINK_FLAGS} CFLAGS ${CANLIB_INCLUDE_FLAGS}
    SOURCES segwayrmp.cc canio_kvaser.cc usb_packet.cc)
ELSE (${HAVE_CANLIB})
PLAYERDRIVER_ADD_DRIVER (segwayrmp build_segwayrmp
    SOURCES segwayrmp.cc usb_packet.cc)
ENDIF (${HAVE_CANLIB})
