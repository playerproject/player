SET (playerutilSrcs localization.c)
INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)

PLAYER_ADD_LIBRARY (playerutil ${playerutilSrcs})
PLAYER_MAKE_PKGCONFIG ("playerutil" "Player utility functions library - part of the Player Project"
                       "" "" "" "")
ADD_DEPENDENCIES (playerutil playerc)

PLAYER_INSTALL_HEADERS (playerutil localization.h)

