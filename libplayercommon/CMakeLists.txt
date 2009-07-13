SET (playercommonSrcs error.c)
PLAYER_ADD_LIBRARY (playercommon ${playercommonSrcs})
PLAYER_MAKE_PKGCONFIG ("playercommon" "Player error reporting and utility library - part of the Player Project"
                       "" "" "" "")
TARGET_LINK_LIBRARIES (playercommon)

PLAYER_INSTALL_HEADERS (playercommon playercommon.h ${playerconfig_h} error.h)
