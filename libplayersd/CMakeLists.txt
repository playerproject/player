IF (HAVE_DNS_SD)
    OPTION (BUILD_PLAYERSD "Build libplayersd" OFF)
ENDIF (HAVE_DNS_SD)

IF (BUILD_PLAYERSD)
    SET(HAVE_PLAYERSD true)
    SET (playersdSrcs playersd_mdns.c playersd.c)

    PLAYER_ADD_LIBRARY (playersd ${playersdSrcs})
    TARGET_LINK_LIBRARIES (playersd playerinterface playercommon)
    TARGET_LINK_LIBRARIES(playersd dns_sd)
    IF (PTHREAD_LIB)
        SET (PTHREAD_LIB_FLAG "-l${PTHREAD_LIB}")
    ENDIF (PTHREAD_LIB)
    PLAYER_MAKE_PKGCONFIG ("playersd" "Player service discovery library - part of the Player Project" "playerinterface playercommon" "" "" "-ldns_sd ${PTHREAD_LIB_FLAG}")

    PLAYER_INSTALL_HEADERS (playersd playersd.h)
ENDIF (BUILD_PLAYERSD)
