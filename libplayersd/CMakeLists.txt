IF (HAVE_DNS_SD)
    OPTION (HAVE_PLAYERSD "Build libplayersd" OFF)
ENDIF (HAVE_DNS_SD)

IF (HAVE_PLAYERSD)
    SET (playersdSrcs playersd_mdns.c playersd.c)

    PLAYER_ADD_LIBRARY (playersd ${playersdSrcs})
    TARGET_LINK_LIBRARIES (playersd playererror playerutils)
    PLAYER_ADD_LINK_LIB (dns_sd)
    PLAYER_MAKE_PKGCONFIG ("playersd" "Player service discovery library - part of the Player Project" "playererror playerutils" "" "" "-ldns_sd -lpthread")
ENDIF (HAVE_PLAYERSD)