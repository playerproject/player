SET (playercommonSrcs error.c)

PLAYER_ADD_LIBRARY (playercommon ${playercommonSrcs})
PLAYER_MAKE_PKGCONFIG ("playercommon" "Player error reporting and utility library - part of the Player Project"
                       "" "" "" "")

PLAYER_INSTALL_HEADERS (playercommon playercommon.h error.h)

