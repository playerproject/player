OPTION (INCLUDE_TCP "Include the TCP transport layer" ON)
OPTION (INCLUDE_UDP "Include the UDP transport layer" ON)

INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR}/libplayercore)

IF (INCLUDE_TCP)
    OPTION (ENABLE_TCP_NODELAY "Turn off Nagel's buffering algorithm (which may increase socket latency when used)" OFF)

    SET (playertcpSrcs socket_util.c playertcp.cc remote_driver.cc)
    PLAYER_ADD_LIBRARY (playertcp ${playertcpSrcs})
    TARGET_LINK_LIBRARIES (playertcp replace playercore playererror playerutils playerxdr)
    TARGET_LINK_LIBRARIES (playertcp z)
    PLAYER_ADD_LINK_LIB (z)
    PLAYER_MAKE_PKGCONFIG ("playertcp" "Player TCP messaging library - part of the Player Project" "playererror playercore" "" "" "")

    PLAYER_INSTALL_HEADERS (playertcp playertcp.h)
ENDIF (INCLUDE_TCP)

IF (INCLUDE_UDP)
    SET (playerudpSrcs playerudp.cc)
    PLAYER_ADD_LIBRARY (playerudp ${playerudpSrcs})
    TARGET_LINK_LIBRARIES (playerudp replace playercore playererror playerutils playerxdr)
    TARGET_LINK_LIBRARIES (playerudp z)
    PLAYER_MAKE_PKGCONFIG ("playerudp" "Player UDP messaging library - part of the Player Project" "playererror playercore" "" "" "")

    PLAYER_INSTALL_HEADERS (playertcp playerudp.h)
ENDIF (INCLUDE_UDP)
